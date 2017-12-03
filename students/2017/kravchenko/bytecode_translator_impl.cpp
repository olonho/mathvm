#include "mathvm.h"
#include "parser.h"
#include "visitors.h"
#include "bytecode_translator.h"

#include <iostream>
#include <stdarg.h>

namespace mathvm {

// BytecodeTranslator

Status* BytecodeTranslatorImpl::translate(const string& program, Code* *code)
{
    fprintf(stderr, "here\n");

    Status *res;
    Parser parser;
    res = parser.parseProgram(program);

    if (res->isError())
        return res;

    AstFunction *top = parser.top();

    BytecodeVisitor b_visitor;
    b_visitor.translate(top);

    *code = b_visitor.get_code();

    return res;
}

// BytecodeVisitor

void BytecodeVisitor::registerScopes(Scope *s)
{
    _scope_map[s] = _scope_map.size();

    for (Scope::VarIterator var_it(s); var_it.hasNext();) {
        AstVar *var = var_it.next();
        _var_map[s][var->name()] = _var_map[s].size();
    }

    for (uint32_t i = 0; i < s->childScopeNumber(); i++)
        registerScopes(s->childScopeAt(i));
}

void BytecodeVisitor::registerFunctions(AstFunction *a_fun)
{
    fprintf(stderr, "registering function %s\n", a_fun->name().c_str());
    _funcs.push_back(a_fun);
    _code->addFunction(new BytecodeFunction(a_fun));

    for (Scope::FunctionIterator fun_it(a_fun->node()->body()->scope()); fun_it.hasNext();) {
        AstFunction *fun = fun_it.next();
        registerFunctions(fun);
    }
}

void BytecodeVisitor::translate(AstFunction *a_fun)
{
    fprintf(stderr, "in b_visitor\n");
    // get functions and scopes ids.
    registerScopes(a_fun->scope());
    registerFunctions(a_fun);

    for (AstFunction *fun : _funcs)
        translateAstFunction(fun);
}

void BytecodeVisitor::translateAstFunction(AstFunction *a_fun)
{
    _fun = (BytecodeFunction *)_code->functionByName(a_fun->name());
    _fun->setScopeId(_scope_map[a_fun->scope()]);
    _fun->setLocalsNumber(a_fun->node()->body()->scope()->variablesCount());

    a_fun->node()->visit(this);

    fprintf(stderr, "translated ast function %s:\n", _fun->name().c_str());
    fprintf(stderr, "    id = %d\n", _fun->id());
    fprintf(stderr, "    locals = %d\n", _fun->localsNumber());
    fprintf(stderr, "    params = %d\n", _fun->parametersNumber());
    fprintf(stderr, "    scopeId = %d\n", _fun->scopeId());
}

void BytecodeVisitor::visitBinaryOpNode(BinaryOpNode *node)
{
    /*
    node->left()->visit(this);
    node->right()->visit(this);

    switch (node->kind()) {
        
    }
    */
}

void BytecodeVisitor::visitUnaryOpNode(UnaryOpNode *node)
{
}

void BytecodeVisitor::visitStringLiteralNode(StringLiteralNode *node)
{
}

void BytecodeVisitor::visitIntLiteralNode(IntLiteralNode *node)
{
    _fun->bytecode()->addInsn(BC_ILOAD);
    _fun->bytecode()->addInt64(node->literal());
    types.push(VT_INT);
}

void BytecodeVisitor::visitDoubleLiteralNode(DoubleLiteralNode *node)
{
    _fun->bytecode()->addInsn(BC_DLOAD);
    _fun->bytecode()->addDouble(node->literal());
    types.push(VT_DOUBLE);
}

void BytecodeVisitor::visitLoadNode(LoadNode *node)
{
    fprintf(stderr, "visiting load node for variable %s\n", node->var()->name().c_str());

    const AstVar *var = node->var();
    uint16_t scope_id = _scope_map[var->owner()];
    uint16_t var_id = _var_map[_scope][var->name()];

    if (var->type() == VT_INT) {
        if (scope_id == _scope_map[_scope])
            _fun->bytecode()->addInsn(BC_LOADIVAR);
        else
            _fun->bytecode()->addInsn(BC_LOADCTXIVAR);
        types.push(VT_INT);
    } else if (var->type() == VT_DOUBLE) {
        if (scope_id == _scope_map[_scope])
            _fun->bytecode()->addInsn(BC_LOADDVAR);
        else
            _fun->bytecode()->addInsn(BC_LOADCTXDVAR);
        types.push(VT_DOUBLE);
    } else if (var->type() == VT_STRING) {
        if (scope_id == _scope_map[_scope])
            _fun->bytecode()->addInsn(BC_LOADSVAR);
        else
            _fun->bytecode()->addInsn(BC_LOADCTXSVAR);
        types.push(VT_STRING);
    }

    if (scope_id != _scope_map[_scope])
        _fun->bytecode()->addUInt16(scope_id);
    _fun->bytecode()->addUInt16(var_id);
}

void BytecodeVisitor::convertType(VarType from, VarType to)
{
    if (from == to)
        return;
    if (from == VT_INT && to == VT_DOUBLE) {
        _fun->bytecode()->addInsn(BC_I2D);
        return;
    }
    if (from == VT_DOUBLE && to == VT_INT) {
        _fun->bytecode()->addInsn(BC_D2I);
        return;
    }
    if (from == VT_STRING && to == VT_INT) {
        _fun->bytecode()->addInsn(BC_S2I);
        return;
    }

    fprintf(stderr, "trying to convert from %d to %d\n", from, to);
    assert(false);
}

void BytecodeVisitor::visitStoreNode(StoreNode *node)
{
    fprintf(stderr, "visiting store node for variable %s\n", node->var()->name().c_str());

    const AstVar *var = node->var();
    uint16_t scope_id = _scope_map[var->owner()];
    uint16_t var_id = _var_map[_scope][var->name()];

    if (node->op() == tINCRSET || node->op() == tDECRSET) {
        LoadNode n(0, var);
        n.visit(this);
    }

    node->value()->visit(this);
    convertType(types.top(), var->type());
    types.pop();

    // use STORE?VAR if var belongs to the current scope, STORECTX otherwise
    if (var->type() == VT_INT) {
        if (node->op() == tINCRSET)
            _fun->bytecode()->addInsn(BC_IADD);
        if (node->op() == tDECRSET)
            _fun->bytecode()->addInsn(BC_ISUB);
        if (scope_id == _scope_map[_scope])
            _fun->bytecode()->addInsn(BC_STOREIVAR);
        else
            _fun->bytecode()->addInsn(BC_STORECTXIVAR);
    } else if (var->type() == VT_DOUBLE) {
        if (node->op() == tINCRSET)
            _fun->bytecode()->addInsn(BC_DADD);
        if (node->op() == tDECRSET)
            _fun->bytecode()->addInsn(BC_DSUB);
        if (scope_id == _scope_map[_scope])
            _fun->bytecode()->addInsn(BC_STOREDVAR);
        else
            _fun->bytecode()->addInsn(BC_STORECTXDVAR);
    } else if (var->type() == VT_STRING) {
        if (node->op() == tINCRSET)
            assert(false);
        if (node->op() == tDECRSET)
            assert(false);
        if (scope_id == _scope_map[_scope])
            _fun->bytecode()->addInsn(BC_STORESVAR);
        else
            _fun->bytecode()->addInsn(BC_STORECTXSVAR);
    }

    if (scope_id != _scope_map[_scope])
        _fun->bytecode()->addUInt16(scope_id);
    _fun->bytecode()->addUInt16(var_id);
}

void BytecodeVisitor::visitBlockNode(BlockNode *node)
{
    _scope = node->scope();

    fprintf(stderr, "visiting block for scope %p, scope id = %d\n", _scope, _scope_map[_scope]);
    for (int i = 0; i < (int)node->nodes(); i++) {
        AstNode *child = node->nodeAt(i);
        child->visit(this);
        _scope = node->scope();
    }
}

void BytecodeVisitor::visitNativeCallNode(NativeCallNode *node)
{
}

void BytecodeVisitor::visitForNode(ForNode *node)
{
}

void BytecodeVisitor::visitWhileNode(WhileNode *node)
{
}

void BytecodeVisitor::visitIfNode(IfNode *node)
{
}

void BytecodeVisitor::visitReturnNode(ReturnNode *node)
{
}

void BytecodeVisitor::visitFunctionNode(FunctionNode *node)
{
    // TODO natives
    fprintf(stderr, "visiting function node name %s\n", node->name().c_str());

    node->body()->visit(this);
}

void BytecodeVisitor::visitCallNode(CallNode *node)
{
}

void BytecodeVisitor::visitPrintNode(PrintNode *node)
{
}

}
