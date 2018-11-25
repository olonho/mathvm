//
// Created by Владислав Калинин on 09/11/2018.
//

#ifndef MATHVM_BYTECODETRANSLATORIMPL_H
#define MATHVM_BYTECODETRANSLATORIMPL_H

#include "../../../include/mathvm.h"
#include "../../../vm/parser.h"
#include "printer/print_visitor.h"
#include "translator/BytecodeGenerator.h"
#include "translator/BytecodeInterpeter.h"

using namespace mathvm;

Status *BytecodeTranslatorImpl::translate(const string &program, Code **code) {
    Parser p;
    Status *status = p.parseProgram(program);
    if (status->isOk()) {
        AstFunction *astFunction = p.top();
        auto *ctx = new Context();
        auto *bytecode = new Bytecode();
        auto *translatorVisitor = new BytecodeGenerator(ctx, bytecode);
        try {
            astFunction->node()->visit(translatorVisitor);
        } catch (CompileError e) {
            status = Status::Error(e.getMsg(), e.getPosition());
        }
        *code = new BytecodeInterpeter(bytecode, ctx);
        delete translatorVisitor;
    }
    return status;
}

//BytecodeTranslatorImpl::~BytecodeTranslatorImpl() {
//
//}

#endif //MATHVM_BYTECODETRANSLATORIMPL_H
