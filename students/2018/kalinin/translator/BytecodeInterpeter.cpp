//
// Created by Владислав Калинин on 17/11/2018.
//

#include "BytecodeInterpeter.h"

mathvm::Status *mathvm::BytecodeInterpeter::execute(std::vector<mathvm::Var *> &vars) {
    auto *topFunction = dynamic_cast<BytecodeFunction *>(functionByName("<top>"));
    Bytecode *bytecode = topFunction->bytecode();




    bytecode->getInsn(offset);




    bytecode->dump(cout);

    return Status::Ok();
}

void mathvm::BytecodeInterpeter::interpate(mathvm::Instruction ins, uint32_t offset) {
//    switch (ins) {
//        case :
//    }
}

void mathvm::BytecodeInterpeter::init() {
    ctx = new StackContext();
}
