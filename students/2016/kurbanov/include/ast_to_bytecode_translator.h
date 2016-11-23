#pragma once

#include <memory>

#include "mathvm.h"
#include "parser.h"
#include "ast_to_bytecode_visitor.h"
#include "include/ex/translation_exception.h"

class AstToBytecodeTranslator : public mathvm::Translator {
public:
    typedef std::shared_ptr<AstToBytecodeVisitor> VisitorPtr;

    AstToBytecodeTranslator()
            : mVisitor(new AstToBytecodeVisitor) {}

    virtual Status *translate(const string &program, Code **code) {
        Parser parser;
        Status *status = parser.parseProgram(program);
        if (status->isError()) {
            return status;
        } else {
            delete status;
        }
        try {
            mVisitor->visitTop(parser.top());
        } catch (TranslationException e) {
            return e.errorStatus();
        }
        *code = mVisitor->code();
        return Status::Ok();
    }

private:
    VisitorPtr mVisitor;
};

