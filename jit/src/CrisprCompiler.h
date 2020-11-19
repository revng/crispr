#ifndef CRISPR_CRISPRCOMPILER_H
#define CRISPR_CRISPRCOMPILER_H

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <llvm/ExecutionEngine/Orc/IRTransformLayer.h>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/Triple.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/JITSymbol.h"
#include "llvm/ExecutionEngine/Orc/Core.h"
#include "llvm/ExecutionEngine/Orc/IRCompileLayer.h"
#include "llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Mangler.h"
#include "llvm/Support/DynamicLibrary.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"

#include "CrisprSegmentManager.h"
#include "CrisprMemoryManager.h"
#include "CrisprLinker.h"

class CrisprCompiler {
    using string = std::string;

private:
    llvm::orc::ExecutionSession ES;

    uint64_t CodeSegmentVirtualAddress;
    uint64_t DataSegmentVirtualAddress;
    uint64_t RoDataSegmentVirtualAddress;

    CrisprSegmentManager CSM;

    std::unique_ptr<llvm::TargetMachine> TM;
    const llvm::DataLayout DL;

    llvm::orc::MangleAndInterner Mangler;

    llvm::orc::RTDyldObjectLinkingLayer LinkingLayer;
    CrisprLinker CrisprLinkingLayer;
    llvm::orc::IRCompileLayer CompileLayer;
    llvm::orc::IRTransformLayer OptimizeLayer;
    llvm::orc::IRTransformLayer IsolateSectionsLayer;

    std::vector<std::unique_ptr<CrisprMemoryManager::MemorySegments>> MemorySegmentsV;

    llvm::orc::JITDylib &ExistingSymbolsDylib;

public:
    std::list<std::string> NewFunctions;

    CrisprCompiler(
            string const &TargetTriple,
            uint64_t CodeSegmentVirtualAddress,
            uint64_t DataSegmentVirtualAddress,
            uint64_t RoDataSegmentVirtualAddress);

    CrisprCompiler(CrisprCompiler &&) = delete;

    CrisprCompiler &operator=(CrisprCompiler &&) = delete;

    llvm::Error addModule(llvm::orc::ThreadSafeModule M);

    llvm::Error addObject(std::unique_ptr<llvm::MemoryBuffer> O);

    llvm::Error addExistingSymbols(const std::map<string, uint64_t> &Symbols);

    llvm::Error addExistingSymbols(const std::map<string, uint64_t> &Symbols, llvm::JITSymbolFlags flags);

    llvm::Expected<llvm::JITSymbol> findSymbol(llvm::StringRef Name);

    static llvm::Expected<llvm::orc::ThreadSafeModule>
    optimizeModule(llvm::orc::ThreadSafeModule M, const llvm::orc::MaterializationResponsibility &R);

    static llvm::Expected<llvm::orc::ThreadSafeModule>
    isolateSections(llvm::orc::ThreadSafeModule M, const llvm::orc::MaterializationResponsibility &R);

    void dumpSegments(const string &to_dir);


private:
    std::unique_ptr<llvm::RuntimeDyld::MemoryManager> getMemoryManager();

    static llvm::TargetOptions getTargetOptions() {
        llvm::TargetOptions TO;
        TO.FunctionSections = true;
        TO.DataSections = true;
        TO.UniqueSectionNames = true;
        return TO;
    }
};

#endif//CRISPR_CRISPRCOMPILER_H