#include <vector>
#include <iostream>
#include <string>

#include "llvm/IR/Module.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/FileCheck.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/Error.h"

#include "CrisprCompiler.h"
#include "CrisprLinker.h"
#include "ArgParser.h"
#include "CSVReader.h"
#include "util.h"

using namespace llvm;
using namespace std;
using namespace llvm::orc;

void InitTarget() {
    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();
    InitializeNativeTargetAsmParser();
}

void add_static_libraries(const string &paths, CrisprCompiler &Recompiler) {
    auto libraries = split(paths, ",");

    for (const auto &lib: libraries) {
        auto MB = llvm::MemoryBuffer::getFile(lib);
        if (MB.getError()) {
            errs() << "Could not open " << lib << "\n";
            exit(1);
        } else {
            auto Err = Recompiler.addObject(std::move(*MB));
            if (Err) {
                errs() << "Error while adding library: " << Err << "\n";
                exit(1);
            }
        }
    }
}

ThreadSafeModule ParseModule(const string &path) {
    SMDiagnostic Err;

    ThreadSafeContext TSCtx(std::make_unique<LLVMContext>());

    unique_ptr<Module> M = parseIRFile(path, Err, *TSCtx.getContext());
    if (!M) {
        errs() << "Could not parse IR file. The error was:\n";
        errs() << Err.getMessage();
        exit(1);
    }

    std::string buffer;
    raw_string_ostream es(buffer);
    if (verifyModule(*M, &es)) {
        cerr << "Module verification failed: " << es.str().c_str();
    }

    return ThreadSafeModule(std::move(M), TSCtx);
}

void AddSymbolsFromCSV(const string &symbols_path, CrisprCompiler &Recompiler) {
    std::map<string, uint64_t> Symbols;
    ifstream symbols_file(symbols_path);
    CSVRow row;
    while (symbols_file >> row) {
        string SymbolName = row[0];
        uint64_t Address = std::stoull(row[1], nullptr, 16);
        uint64_t Size = std::stoull(row[2], nullptr, 16);
        Size += 0;
        Symbols[SymbolName] = Address;
    }
    Error Err = Recompiler.addExistingSymbols(Symbols);
    if (Err) {
        errs() << "Error while adding existing symbols: " << Err;
        exit(1);
    }
}

std::shared_ptr<std::map<string, uint64_t>>
lookup_new_symbols(CrisprCompiler &Recompiler) {
    auto lookup_results = std::make_shared<std::map<string, uint64_t>>();

    for (auto const &symbol: Recompiler.NewFunctions) {
        errs() << "Looking up " << symbol << "\n";
        auto S = Recompiler.findSymbol(symbol);
        if (!S) {
            errs() << "Could not look up address of " << symbol
                   << ": " << S.takeError() << "\n";
            exit(1);
        }

        JITTargetAddress Addr = cantFail(S->getAddress());
        outs() << "Looked up address of " << symbol << ": " << format_hex(reinterpret_cast<uint64_t>(Addr), 10) << "\n";
        (*lookup_results)[symbol] = Addr;
    }
    return lookup_results;
}

int main(int argc, char **argv) {
    InputParser Parser(argc, argv);
    string module_path = Parser.getCmdOption("-m");
    uint64_t code_vaddr = std::stoull(Parser.getCmdOption("--map-code-to", "a000"), nullptr, 16);
    uint64_t rodata_vaddr = std::stoull(Parser.getCmdOption("--map-rodata-to", "b000"), nullptr, 16);
    uint64_t data_vaddr = std::stoull(Parser.getCmdOption("--map-data-to", "c000"), nullptr, 16);
    string symbols_file_path = Parser.getCmdOption("--symbols");
    string static_libs_file_paths = Parser.getCmdOption("--static-link-libs");
    string export_new_symbols_to = Parser.getCmdOption("--export-to");

    // Initialize the JIT
    InitTarget();

    const string TargetTriple = "x86_64-unknown-linux-gnu";
    CrisprCompiler CrisprCompiler(TargetTriple, code_vaddr, data_vaddr, rodata_vaddr);

    // Add new static libraries
    add_static_libraries(static_libs_file_paths, CrisprCompiler);

    // Get the module to be compiled
    ThreadSafeModule M = ParseModule(module_path);

    // TODO: read the symbols from the target binary
    // Add pre-existing symbols provided by the user
    AddSymbolsFromCSV(symbols_file_path, CrisprCompiler);

    // Add the module to be compiled
    Error Err = CrisprCompiler.addModule(std::move(M));
    if (Err) {
        errs() << "Error while adding module: " << Err;
        exit(1);
    }

    // Lookup symbols to export
    // This will trigger compilation of those symbols and their dependencies
    auto exported_symbols = lookup_new_symbols(CrisprCompiler);

    // Export symbols for the patcher
    ofstream exported_symbols_file(export_new_symbols_to);
    for (const auto &ExportedSymbol: *exported_symbols) {
        string SymbolName = ExportedSymbol.first;
        uint64_t SymbolAddress = ExportedSymbol.second;
        exported_symbols_file << SymbolName << "," << SymbolAddress << "\n";
    }
    exported_symbols_file.close();

    CrisprCompiler.dumpSegments(std::filesystem::current_path() / "tmp");

    return 0;
}
