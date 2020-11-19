#ifndef CRISPR_CRISPRMEMORYMANAGER_H
#define CRISPR_CRISPRMEMORYMANAGER_H

#include "llvm/ExecutionEngine/RuntimeDyld.h"

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "CrisprSegmentManager.h"

class CrisprMemoryManager : public llvm::RuntimeDyld::MemoryManager {
    using StringRef = llvm::StringRef;
    using string = std::string;
    using RuntimeDyld = llvm::RuntimeDyld;
    using ObjectFile = llvm::object::ObjectFile;

public:
    struct MemorySegments {
        uint8_t *CodeSegment;
        uint8_t *DataSegment;
        uint8_t *RoDataSegment;
        size_t CodeSegmentSize;
        size_t DataSegmentSize;
        size_t RoDataSegmentSize;
    };

    enum SectionAttributes {
        NONE = 0,
        EXECUTABLE = 1u << 0u,
        WRITABLE = 1u << 1u
    };

    struct SectionAllocation {
        uint8_t *LocalAddress;
        uint64_t TargetProcessAddress;
        size_t Size;
        uint32_t Attributes;
    };

    CrisprMemoryManager(
            CrisprSegmentManager &CSM,
            MemorySegments &MemorySegments) : CSM(CSM),
                                              MS(MemorySegments),
                                              CodeSegmentNextFreeOffset(0),
                                              DataSegmentNextFreeOffset(0),
                                              RoDataSegmentNextFreeOffset(0) {}

    ~CrisprMemoryManager() override = default;

    uint8_t *
    allocateCodeSection(uintptr_t Size, unsigned Alignment, unsigned SectionID, StringRef SectionName) override;

    uint8_t *
    allocateDataSection(uintptr_t Size, unsigned Alignment, unsigned SectionID, StringRef SectionName, bool IsReadOnly) override;

    void notifyObjectLoaded(RuntimeDyld &Dyld, const ObjectFile &Obj) override;

    void reserveAllocationSpace(uintptr_t CodeSize, uint32_t CodeAlign, uintptr_t RODataSize, uint32_t RODataAlign,
                                uintptr_t RWDataSize, uint32_t RWDataAlign) override;

    bool needsToReserveAllocationSpace() override { return true; }

    // TODO
    void registerEHFrames(uint8_t *Addr, uint64_t LoadAddr, size_t Size) override {}

    // TODO
    void deregisterEHFrames() override {}

    bool finalizeMemory(string *ErrMsg) override { return false; }

    [[nodiscard]] uint8_t *getCodeSegment() const { return MS.CodeSegment; }

    [[nodiscard]] uint8_t *getDataSegment() const { return MS.DataSegment; }

    [[nodiscard]] uint8_t *getRoDataSegment() const { return MS.RoDataSegment; }

    [[nodiscard]] size_t getCodeSegmentSize() const { return MS.CodeSegmentSize; }

    [[nodiscard]] size_t getDataSegmentSize() const { return MS.DataSegmentSize; }

    [[nodiscard]] size_t getRoDataSegmentSize() const { return MS.RoDataSegmentSize; }

private:
    CrisprSegmentManager &CSM;
    MemorySegments &MS;

    size_t CodeSegmentNextFreeOffset;
    size_t DataSegmentNextFreeOffset;
    size_t RoDataSegmentNextFreeOffset;

    std::map<string, SectionAllocation> AllocatedSections;

    uint64_t CodeSegmentTargetProcessBaseVirtAddr;
    uint64_t DataSegmentTargetProcessBaseVirtAddr;
    uint64_t RoDataSegmentTargetProcessBaseVirtAddr;
};

#endif//CRISPR_CRISPRMEMORYMANAGER_H