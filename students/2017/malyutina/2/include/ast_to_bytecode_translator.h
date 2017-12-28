#ifndef AST_TO_BYTECODE_TRANSLATOR_H__
#define AST_TO_BYTECODE_TRANSLATOR_H__

#include <memory>
#include <exception>

#include "../../../../../include/mathvm.h"
#include "../../../../../vm/parser.h"

#include "ast_to_bytecode.h"

using namespace mathvm;

class ast_to_bytecode_translator : public Translator {
public:
    typedef std::shared_ptr<ast_to_bytecode> ptr_ast_to_byte_code;

    ast_to_bytecode_translator() : _visitor(new ast_to_bytecode) {}

    virtual Status *translate(const string &program, Code **code) {
        Parser parser;
        Status *status = parser.parseProgram(program);
        if (status->isError()) {
            return status;
        } else {
            delete status;
        }
        try {
            _visitor->visitTop(parser.top());
        } catch (translate_exception ex) {
            return ex.errorStatus();
        }
        *code = _visitor->code();
        return Status::Ok();
    }

private:
    ptr_ast_to_byte_code _visitor;
};

#endif
