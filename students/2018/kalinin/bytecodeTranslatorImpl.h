//
// Created by Владислав Калинин on 09/11/2018.
//

#ifndef MATHVM_BYTECODETRANSLATORIMPL_H
#define MATHVM_BYTECODETRANSLATORIMPL_H

#include "../../../include/mathvm.h"
#include "../../../vm/parser.h"
#include "printer/print_visitor.h"
#include "bytecode_translator_visitor.h"
#include "BytecodeInterpeter.h"

using namespace mathvm;

Status *BytecodeTranslatorImpl::translate(const string &program, Code **code) {
    Parser p;
    Status *status = p.parseProgram(program);
    if (status->isOk()) {
        AstFunction *astFunction = p.top();
        *code = new BytecodeInterpeter;
        auto *translatorVisitor = new BytecodeTranslator(*code);
        try {
            astFunction->node()->visit(translatorVisitor);
        } catch (CompileError e) {
            status = Status::Error(e.getMsg(), e.getPosition());
        }
        translatorVisitor->getBytecode()->dump(cout);
        delete translatorVisitor;
    }
    return status;
}

//BytecodeTranslatorImpl::~BytecodeTranslatorImpl() {
//
//}

#endif //MATHVM_BYTECODETRANSLATORIMPL_H
