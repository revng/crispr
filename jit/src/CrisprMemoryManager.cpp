#include <cstdlib>
#include <fstream>
#include <memory>

#include "CrisprMemoryManager.h"

using llvm::outs;
using llvm::errs;
using llvm::format_hex;

uint8_t *
CrisprMemoryManager::allocateCodeSection(
        uintptr_t Size,
        unsigned Alignment,
        unsigned SectionID,
        StringRef SectionName
) {
    uint8_t *LocalAddress = MS.CodeSegment + CodeSegmentNextFreeOffset;
    uint64_t TargetAddress = CodeSegmentTargetProcessBaseVirtAddr + CodeSegmentNextFreeOffset;
    CodeSegmentNextFreeOffset += Size;

    AllocatedSections[SectionName] = {
            .LocalAddress = LocalAddress,
            .TargetProcessAddress = TargetAddress,
            .Size = Size,
            .Attributes = SectionAttributes::EXECUTABLE
    };

    outs() << "Allocated code section \"" << SectionName << "\""
           << " of size " << format_hex(Size, 6)
           << " at target address " << format_hex(TargetAddress, 10) << "\n";

    return LocalAddress;
}

uint8_t *
CrisprMemoryManager::allocateDataSection(
        uintptr_t Size,
        unsigned Alignment,
        unsigned SectionID,
        StringRef SectionName,
        bool IsReadOnly
) {

    SectionAllocation NewAllocation{
            .Size=Size,
            .Attributes = SectionAttributes::NONE
    };

    if (!IsReadOnly) {
        NewAllocation.LocalAddress = MS.DataSegment + DataSegmentNextFreeOffset;
        NewAllocation.TargetProcessAddress = DataSegmentTargetProcessBaseVirtAddr + DataSegmentNextFreeOffset;
        DataSegmentNextFreeOffset += Size;
        NewAllocation.Attributes = SectionAttributes::WRITABLE;
    } else {
        NewAllocation.LocalAddress = MS.RoDataSegment + RoDataSegmentNextFreeOffset;
        NewAllocation.TargetProcessAddress = RoDataSegmentTargetProcessBaseVirtAddr + RoDataSegmentNextFreeOffset;
        RoDataSegmentNextFreeOffset += Size;
    }

    AllocatedSections[SectionName] = NewAllocation;

    outs() << "Allocated data section \"" << SectionName << "\""
           << " of size " << format_hex(Size, 6)
           << " at target address " << format_hex(NewAllocation.TargetProcessAddress, 10) << "\n";

    return NewAllocation.LocalAddress;
}

void CrisprMemoryManager::notifyObjectLoaded(RuntimeDyld &Dyld, const ObjectFile &Obj) {
    outs() << "Loaded object with sections:\n";
    for (auto S: Obj.sections()) {
        StringRef SectionName;
        S.getName(SectionName);


        outs() << "\t" << SectionName << " size " << S.getSize() << "\n";

        if (!empty(S.relocations())) {
            outs() << "\t\tRelocations: \n";
        }

        for (auto Rel: S.relocations()) {
            auto SymName = Rel.getSymbol()->getName();
            if (!SymName) {
                outs() << "Error while reading relocation name: " << SymName.takeError() << "\n";
                continue;
            }
            auto SymAddr = Rel.getSymbol()->getAddress();
            if (!SymAddr) {
                outs() << "Error while reading relocation address: " << SymAddr.takeError() << "\n";
                continue;
            }
            auto SymVal = Rel.getSymbol()->getValue();

            llvm::SmallVector<char, 20> RelTypeName;
            Rel.getTypeName(RelTypeName);

            outs() << "\t\t"
                   << RelTypeName << " "
                   << SymName.get() << ", "
                   << SymAddr.get() << ", "
                   << SymVal << ", "
                   << Rel.getOffset()
                   << "\n";
        }
    }

    outs() << "Defined symbols: \n";
    for (auto S: Obj.symbols()) {
        auto Name = S.getName();
        auto Address = S.getAddress();
        auto Type = S.getType();

        if (!Name) {
            errs() << "Error while reading symbol: " << Name.takeError() << "\n";
            continue;
        }

        if (!Address) {
            errs() << "Error while reading symbol: " << Address.takeError() << "\n";
            continue;
        }

        if (!Type) {
            errs() << "Error while reading symbol: " << Type.takeError() << "\n";
            continue;
        }

        outs() << "\t"
               << "Name: " << *Name
               << ", addr: " << *Address
               << ", type: " << *Type
               << "\n";
    }

    for (auto const &S: AllocatedSections) {
        string SectionName = S.first;
        SectionAllocation Allocation = S.second;
        outs() << "Remapping section " << SectionName
               << " to " << format_hex(Allocation.TargetProcessAddress, 10) << "\n";
        Dyld.mapSectionAddress(Allocation.LocalAddress, Allocation.TargetProcessAddress);
    }
}


void CrisprMemoryManager::reserveAllocationSpace(uintptr_t CodeSize,
                                                 uint32_t CodeAlign,
                                                 uintptr_t RODataSize,
                                                 uint32_t RODataAlign,
                                                 uintptr_t RWDataSize,
                                                 uint32_t RWDataAlign) {
    outs() << "Request to allocate \n"
           << "\t" << format_hex(CodeSize, 10) << " for code\n"
           << "\t" << format_hex(RODataSize, 10) << " for rodata\n"
           << "\t" << format_hex(RWDataSize, 10) << " for data\n\n";

    MS.CodeSegment = reinterpret_cast<uint8_t *>(malloc(CodeSize));
    MS.CodeSegmentSize = CodeSize;
    if (MS.CodeSegment == nullptr) {
        errs() << "Cannot allocate code segment";
        exit(1);
    }

    MS.DataSegment = reinterpret_cast<uint8_t *>(malloc(RWDataSize));
    MS.DataSegmentSize = RWDataSize;
    if (MS.DataSegment == nullptr) {
        errs() << "Cannot allocate data segment";
        exit(1);
    }

    MS.RoDataSegment = reinterpret_cast<uint8_t *>(malloc(RODataSize));
    MS.RoDataSegmentSize = RODataSize;
    if (MS.RoDataSegment == nullptr) {
        errs() << "Cannot allocate rodata segment";
        exit(1);
    }

    CodeSegmentTargetProcessBaseVirtAddr = CSM.requestCodeAddr(CodeSize);
    DataSegmentTargetProcessBaseVirtAddr = CSM.requestDataAddr(RWDataSize);
    RoDataSegmentTargetProcessBaseVirtAddr = CSM.requestRoDataAddr(RODataSize);
}
