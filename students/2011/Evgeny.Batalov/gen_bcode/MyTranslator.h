#pragma once
#include <string>
#include <mathvm.h>
#include <ast.h>
#include "MyCode.h"
#include "parse.h"
#include "CodeVisitor.h"

class MyTranslator: public mathvm::Translator {
    mathvm::Status status;

public:
    virtual mathvm::Status* translate(const std::string& program, mathvm::Code* *code) {
        try {
            status = mathvm::Status();
            mathvm::Parser p;
            parseExpr(p, program);
            CodeVisitor v(dynamic_cast<mathvm::BlockNode*>(p.top()));
            v.translate();
            v.getCode().dump();
            *code = &v.getCode();
        }
        catch (TranslationException ex) {
            status = mathvm::Status(ex.what());
        }
        return &status;
    }
};
