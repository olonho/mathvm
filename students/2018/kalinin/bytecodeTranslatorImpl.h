//
// Created by Владислав Калинин on 09/11/2018.
//

#ifndef MATHVM_BYTECODETRANSLATORIMPL_H
#define MATHVM_BYTECODETRANSLATORIMPL_H

#include "../../../include/mathvm.h"
#include "../../../vm/parser.h"
#include "printer/print_visitor.h"
#include "bytecode_translator_visitor.h"

using namespace mathvm;

Status *BytecodeTranslatorImpl::translate(const string &program, Code **code) {
    Parser p;
    Status *status = p.parseProgram(program);
    if (status->isOk()) {
        AstFunction *astFunction = p.top();
        auto *translator_visitor = new Bytecode_translator_visitor();
        try {
            astFunction->node()->visit(translator_visitor);
        } catch (CompileError e) {
            status = Status::Error(e.getMsg(), e.getPosition());
        }
        delete translator_visitor;
    }
    cout << status->isError() << endl;
    return status;
}

//BytecodeTranslatorImpl::~BytecodeTranslatorImpl() {
//
//}

#endif //MATHVM_BYTECODETRANSLATORIMPL_H
