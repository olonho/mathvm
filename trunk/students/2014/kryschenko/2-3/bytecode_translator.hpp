#ifndef BYTECODE_TRANSLATOR_HPP
#define BYTECODE_TRANSLATOR_HPP

#include "ast.h"
#include "mathvm.h"
#include "visitors.h"
#include "parser.h"
#include "scope_context.hpp"

namespace mathvm {

    class BytecodeVisitor : public AstVisitor {
    private:
        Code* code_;
        ScopeContext* context_;

    public:
        BytecodeVisitor():
            code_(NULL),
            context_(NULL) {

        }
        ~BytecodeVisitor() {
            if (context_ != NULL) {
                delete context_;
            }
        }

        #define VISITOR_FUNCTION(type, name) \
            /*virtual*/ void visit##type(type* node);
            FOR_NODES(VISITOR_FUNCTION)
        #undef VISITOR_FUNCTION

        Status* translateBytecode(Code* code, AstFunction* top);


    private:

        Bytecode* bc() {
            return context_->getBf()->bytecode();
        }
        VarType tosType() {
            return context_->getTosType();
        }
        void setTosType(VarType t) {
            context_->setTosType(t);
        }

        void addArithmeticalOp(BinaryOpNode * pNode);
        void addLogicalOp(BinaryOpNode * pNode);
        void addCompareOp(BinaryOpNode * pNode);
        void addBitwiseOp(BinaryOpNode * pNode);
        void addNotOp(UnaryOpNode * pNode);
        void addUnarySubOp(UnaryOpNode * pNode);
        void castForArithmeticalOp(VarType type, VarType varType);
        void translateFunction(AstFunction * f);
        void castTos(VarType type);
        void loadVar(AstVar const * var);
        void storeVar(AstVar const * var);
    };

    class BytecodeTranslator : public Translator {
    public:
        /* virtual*/ Status* translate(const string& program, Code* *code);

    };
}

#endif