//
// Created by Владислав Калинин on 17/11/2018.
//

#include "BytecodeInterpeter.h"

mathvm::Status *mathvm::BytecodeInterpeter::execute(std::vector<mathvm::Var *> &vars) {
    auto *topFunction =  dynamic_cast<BytecodeFunction*>(functionByName("<top>"));
    topFunction->bytecode()->dump(cout);
    return Status::Ok();
}
