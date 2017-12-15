#ifndef __BYTECODE_TRANSLATOR_IMPL_H__
#define __BYTECODE_TRANSLATOR_IMPL_H__

#include "ast.h"
#include "mathvm.h"
#include "parser.h"
#include "visitors.h"
#include "bytecode_interpreter.h"

#include <iostream>
#include <map>
#include <vector>
#include <stack>

#include <dlfcn.h>

namespace mathvm {

class BytecodeVisitor : public AstBaseVisitor
{
private:
    typedef std::map<Scope*, uint16_t> ScopeMap;
    typedef std::map<Scope*, std::map<string, uint16_t>> VarMap;
    typedef std::map<string, uint16_t> FunIdMap;

    BytecodeInterpreter *_code; // DO NOT delete it in distructor

    BytecodeFunction *_fun; // current function we translate
    Scope *_scope; // current scope

    ScopeMap _scope_map;
    VarMap _var_map;
    FunIdMap _funIdMap;

    std::vector<AstFunction *> _funcs;

    std::stack<VarType> types;
    std::stack<size_t> _scopeSizes;

    void *_dlHandler;

public:
    virtual ~BytecodeVisitor() {
        dlclose(_dlHandler);
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

    void binaryMathOp(BinaryOpNode *node);
    void binaryCompareOp(BinaryOpNode *node);
    void binaryLogicOp(BinaryOpNode *node);

    void addInsn(Instruction insn);
    void addBranch(Instruction insn, Label &l);

    void enterScope();
    void leaveScope();

    bool isNative(FunctionNode *node);
};

}

#endif // __AST_TRANSLATOR_IMPL_H__
