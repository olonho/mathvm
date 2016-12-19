#ifndef M_TRANSLATOR_H
#define M_TRANSLATOR_H

#include "ast.h"
#include "mathvm.h"
#include "visitors.h"
#include "parser.h"
#include "scope_context.h"

namespace mathvm {

class BytecodeVisitor : public AstVisitor {
private:
    Code *_code;
    ScopeContext *_context;

public:
    BytecodeVisitor():
        _code(nullptr),
        _context(nullptr) {

    }
    ~BytecodeVisitor() {
        if (_context != nullptr) {
            delete _context;
        }
    }

#define VISITOR_FUNCTION(type, name) \
    void visit##type(type *node);
    FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION

    Status* translateToBytecode(Code *code, AstFunction *top);

private:

    Bytecode* bytecode() {
        return _context->getByteFunc()->bytecode();
    }
    VarType tosType() {
        return _context->getTosType();
    }
    void setTosType(VarType t) {
        _context->setTosType(t);
    }

    void loadVar(AstVar const *var);
    void storeVar(AstVar const *var);

    void handleArithOp(BinaryOpNode *op_node);
    void handleLogicOp(BinaryOpNode *op_node);
    void handleCmpOp(BinaryOpNode *op_node);
    void handleBitOp(BinaryOpNode *op_node);
    void handleNotOp(UnaryOpNode *op_node);
    void handleNegateOp(UnaryOpNode *op_node);

    void translateFunction(AstFunction *ast_func);

    void castTos(VarType);
    void castArithOp(VarType, VarType);
};

class TranslatorToBytecode : public Translator {
public:
    Status* translate(string const &program, Code **code);
};
}

#endif // M_TRANSLATOR_H
