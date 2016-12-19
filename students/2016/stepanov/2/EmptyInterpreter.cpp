//
// Created by user on 10/29/16.
//

#include "EmptyInterpreter.h"

mathvm::Status *mathvm::EmptyInterpreter::execute(std::vector<mathvm::Var *> &vars) {
    return Status::Ok();
}

