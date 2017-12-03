#ifndef __BYTECODE_TRANSLATOR_IMPL_H__
#define __BYTECODE_TRANSLATOR_IMPL_H__

#include "ast.h"
#include "mathvm.h"
#include "parser.h"
#include "visitors.h"
#include "interpreter_code.h"

#include <iostream>
#include <map>
#include <vector>
#include <stack>

namespace mathvm {

class BytecodeVisitor : public AstBaseVisitor
{
private:
    typedef std::map<Scope*, uint16_t> ScopeMap;
    typedef std::map<Scope*, std::map<string, uint16_t>> VarMap;

    Code *_code; // DO NOT delete it in distructor

    BytecodeFunction *_fun; // current function we translate
    Scope *_scope; // current scope

    ScopeMap _scope_map;
    VarMap _var_map;

    std::vector<AstFunction *> _funcs;

    std::stack<VarType> types;

public:
    BytecodeVisitor() {
        _code = new InterpreterCodeImpl();
    }

    virtual ~BytecodeVisitor() {
    }

    Code * get_code() {
        return _code;
    }

    void registerScopes(Scope *s);
    void registerFunctions(AstFunction *a_fun);

    void translate(AstFunction *a_fun);
    void translateAstFunction(AstFunction *a_fun);

#define VISITOR_FUNCTION(type, name) \
    virtual void visit##type(type* node);

    FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION

private:
    void convertType(VarType to);
    std::vector<uint8_t> opResType(TokenKind op);
    void correctTypes(int n, std::vector<uint8_t> resTypes);

    void binaryMathOp(TokenKind op);
};

}

#endif // __AST_TRANSLATOR_IMPL_H__
