//
// Created by fcremo on 10/30/20.
//

#ifndef CRISPR_CRISPRLINKER_H
#define CRISPR_CRISPRLINKER_H

#include <filesystem>
#include <fstream>

#include "llvm/ExecutionEngine/Orc/Core.h"

class CrisprLinker : public llvm::orc::ObjectLayer {
private:
    llvm::orc::ObjectLayer &RealLinker;
    uint LinkedCount;

public:
    CrisprLinker(
            llvm::orc::ExecutionSession &es,
            llvm::orc::ObjectLayer &RealLinker) : ObjectLayer(es),
                                                  RealLinker(RealLinker),
                                                  LinkedCount(0) {}

    ~CrisprLinker() override = default;

    llvm::Error add(
            llvm::orc::JITDylib &JD,
            std::unique_ptr<llvm::MemoryBuffer> O,
            llvm::orc::VModuleKey K = llvm::orc::VModuleKey()) override {
        return RealLinker.add(JD, std::move(O), K);
    }

    void emit(llvm::orc::MaterializationResponsibility R, std::unique_ptr<llvm::MemoryBuffer> O) override {
        std::filesystem::path out_path("tmp/obj_" + std::to_string(LinkedCount));
        llvm::outs() << "CrisprLinker::emit() called, dumping object file to " << out_path << "\n";
        std::ofstream out(out_path);
        out << O->getBuffer().str();
        out.close();
        LinkedCount++;

        return RealLinker.emit(std::move(R), std::move(O));
    }
};


#endif//CRISPR_CRISPRLINKER_H
