#include <memory>
#include <utility>

#include "llvm/ExecutionEngine/Orc/CompileUtils.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Transforms/Scalar.h"

#include "CrisprCompiler.h"
#include "SeparateFunctionsPass.h"

using namespace llvm;
using namespace llvm::orc;
using namespace std;
namespace fs = std::filesystem;

CrisprCompiler::CrisprCompiler(
        string const &TargetTriple,
        uint64_t CodeSegmentVirtualAddress,
        uint64_t DataSegmentVirtualAddress,
        uint64_t RoDataSegmentVirtualAddress
) :
        CodeSegmentVirtualAddress(CodeSegmentVirtualAddress),
        DataSegmentVirtualAddress(DataSegmentVirtualAddress),
        RoDataSegmentVirtualAddress(RoDataSegmentVirtualAddress),
        CSM(CodeSegmentVirtualAddress, DataSegmentVirtualAddress, RoDataSegmentVirtualAddress),
        TM(llvm::EngineBuilder()
                   .setVerifyModules(true)
                   .setRelocationModel(Reloc::Model::PIC_)
                   .setCodeModel(CodeModel::Small)
                   .setTargetOptions(getTargetOptions())
                   .selectTarget(
                           llvm::Triple(TargetTriple),
                           "",
                           "generic",
                           llvm::SmallVector<std::string, 0>({}))
        ),
        DL(TM->createDataLayout()),
        Mangler(ES, DL),

        LinkingLayer(ES, [this]() { return getMemoryManager(); }),
        CrisprLinkingLayer(ES, LinkingLayer),

        CompileLayer(ES, CrisprLinkingLayer, llvm::orc::SimpleCompiler(*TM)),
        OptimizeLayer(ES, CompileLayer, optimizeModule),
        IsolateSectionsLayer(ES, OptimizeLayer, isolateSections),

        ExistingSymbolsDylib(ES.createJITDylib("PreExistingSymbols", false)) {

    // LinkingLayer.setProcessAllSections(true);
    ES.getMainJITDylib().addToSearchOrder(ExistingSymbolsDylib, true);
}

Error CrisprCompiler::addModule(ThreadSafeModule M) {
    for (auto const &F : M.getModule()->functions()) {
        errs() << "New function: " << F.getName() << "\n";
        if (!F.isDeclaration()) NewFunctions.push_back(F.getName());
    }
    return IsolateSectionsLayer.add(ES.getMainJITDylib(), std::move(M));
}

Error CrisprCompiler::addObject(std::unique_ptr<llvm::MemoryBuffer> O) {
    return CrisprLinkingLayer.add(ES.getMainJITDylib(), std::move(O));
}

Error CrisprCompiler::addExistingSymbols(const std::map<string, uint64_t> &Symbols) {
    return addExistingSymbols(
            Symbols,
            llvm::JITSymbolFlags::Weak
    );
}

Error CrisprCompiler::addExistingSymbols(const map<string, uint64_t> &Symbols, JITSymbolFlags flags) {
    SymbolMap SM;
    for (const auto &S : Symbols) {
        auto const &SymbolName = S.first;
        auto const &SymbolAddr = S.second;
        dbgs() << "Defining symbol " << SymbolName << " as " << format_hex(SymbolAddr, 10) << "\n";
        auto InternedName = ES.intern(SymbolName);
        SM[InternedName] = JITEvaluatedSymbol(SymbolAddr, flags);
    }
    return ExistingSymbolsDylib.define(absoluteSymbols(SM));
}

Expected<JITSymbol> CrisprCompiler::findSymbol(StringRef Name) {
    JITDylib &MainJITDylib = ES.getMainJITDylib();
    std::vector<JITDylib *> SearchOrder({&MainJITDylib});
    return ES.lookup(SearchOrder, Mangler(Name));
}

std::unique_ptr<RuntimeDyld::MemoryManager> CrisprCompiler::getMemoryManager() {
    MemorySegmentsV.push_back(std::make_unique<CrisprMemoryManager::MemorySegments>());
    return std::make_unique<CrisprMemoryManager>(CSM, *MemorySegmentsV.back());
}

void CrisprCompiler::dumpSegments(const string &to_dir) {
    fs::path dir_path(to_dir);

    int i = 0;
    for (auto const &MemSegment: MemorySegmentsV) {
        ofstream outfile;
        outs() << "Dumping code segment\n";
        outfile.open(dir_path / ("code_" + std::to_string(i)));
        outfile.seekp(0, std::ofstream::end);
        outfile.write(reinterpret_cast<char *>(MemSegment->CodeSegment), MemSegment->CodeSegmentSize);
        outfile.close();

        if (MemSegment->DataSegmentSize) {
            outs() << "Dumping data segment\n";
            outfile.open(dir_path / ("data_" + std::to_string(i)));
            outfile.seekp(0, std::ofstream::end);
            outfile.write(reinterpret_cast<char *>(MemSegment->DataSegment), MemSegment->DataSegmentSize);
            outfile.close();
        } else { outs() << "Empty data segment, skipping\n"; }

        if (MemSegment->RoDataSegmentSize) {
            outs() << "Dumping rodata segment\n";
            outfile.open(dir_path / ("rodata_" + std::to_string(i)));
            outfile.seekp(0, std::ofstream::end);
            outfile.write(reinterpret_cast<char *>(MemSegment->RoDataSegment), MemSegment->RoDataSegmentSize);
            outfile.close();
        } else { outs() << "Empty rodata segment, skipping\n"; }
        i++;
    }
}

Expected<ThreadSafeModule>
CrisprCompiler::optimizeModule(ThreadSafeModule M, const MaterializationResponsibility &R) {
    // Create a function pass manager.
    auto FPM = std::make_unique<legacy::FunctionPassManager>(M.getModule());

    // Add some optimizations.
    FPM->add(createInstructionCombiningPass());
    FPM->add(createReassociatePass());
    FPM->add(createGVNPass());
    FPM->add(createCFGSimplificationPass());
    FPM->doInitialization();

    // Run the optimizations over all functions in the module being added to
    // the JIT.
    for (auto &F : *M.getModule()) {
        FPM->run(F);
    }
    return M;
}

Expected<ThreadSafeModule>
CrisprCompiler::isolateSections(ThreadSafeModule M, const MaterializationResponsibility &R) {

    // Create a function pass manager.
    auto FPM = std::make_unique<legacy::FunctionPassManager>(M.getModule());

    // Add some optimizations.
    FPM->add(new SeparateFunctionsPass());
    FPM->doInitialization();

    // Run the optimizations over all functions in the module being added to
    // the JIT.
    for (auto &F : *M.getModule()) {
        FPM->run(F);
    }
    return M;

}

