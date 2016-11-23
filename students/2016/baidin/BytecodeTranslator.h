#pragma once

#include "ast.h"
#include "mathvm.h"
#include "stack"

namespace mathvm {


    class TranslationException: public runtime_error {
    public:
        TranslationException(const string &__arg);

    };

    class ToBytecodeVisitor : public AstVisitor {
        Code *code;
        Bytecode *bytecode;
        TranslatedFunction* curFunction;

        VarType topType;

        map<string, uint16_t> curLocals;
        uint16_t localCount;

    private:

        uint16_t addName(const string &name);

        uint16_t resolveName(const std::string &name);

        void loadVarValue(const std::string &name, VarType varType);

        void storeVarValue(const std::string &name, VarType varType);

        void performArithmeticOperation(VarType type, TokenKind operation);

        void performLogicOperation(VarType type, TokenKind operation);

        void performCmpOperation(VarType type, TokenKind operation);

        void print(AstNode *node);

        void castToType(VarType type);


    public:
        ToBytecodeVisitor(Code *code, Bytecode *bytecode, TranslatedFunction *curFunction);

        virtual ~ToBytecodeVisitor() override;

#define VISITOR_FUNCTION(type, name) \
    void visit##type(type *node);

        FOR_NODES(VISITOR_FUNCTION)

#undef VISITOR_FUNCTION
    };
}
