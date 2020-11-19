//
// Created by fcremo on 11/8/20.
//

#ifndef CRISPR_SEPARATEFUNCTIONSPASS_H
#define CRISPR_SEPARATEFUNCTIONSPASS_H

#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"

class SeparateFunctionsPass : public llvm::FunctionPass {
private:
    char ID;

public:
    bool runOnFunction(llvm::Function &F) override {
        F.setAlignment(1);
        F.setSection(".funcs." + F.getName().str());
        return true;
    }

    SeparateFunctionsPass() : FunctionPass(ID), ID(0) {

    };

    ~SeparateFunctionsPass() override = default;

};

#endif //CRISPR_SEPARATEFUNCTIONSPASS_H
