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

void BytecodeVisitor::convertType(VarType to)
{
    VarType from = types.top();
    types.pop();
    types.push(to);

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

/*
 * This functions tells us which types can arguments of @op have
 * and which types can be returned as result
 *
 * res[0] -- possible types of the result
 * res[1] -- possible types of the first argument
 * res[2] -- possible types of the second argument
 * res[3] -- 0 if arguments can have different types,
 *           1 otherwise
 *
 * if operation only has 1 argument, res[2] can be ignored
 */
std::vector<uint8_t> BytecodeVisitor::opResType(TokenKind op)
{
    std::vector<uint8_t> res;
    res.push_back(0);
    res.push_back(0);
    res.push_back(0);
    res.push_back(0);

    const uint8_t I = 1 << VT_INT;
    const uint8_t D = 1 << VT_DOUBLE;
    const uint8_t S = 1 << VT_STRING;

    switch (op) {
        // there operations only can be applied to integers
        case tAOR:
        case tAAND:
        case tAXOR:
        case tMOD:
            res[0] = res[1] = res[2] = I;
            res[3] = 1;
            break;
        case tNOT:
        case tADD:
        case tSUB:
        case tMUL:
        case tDIV:
            res[0] = res[1] = res[2] = I | D;
            res[3] = 0;
            break;
        case tEQ:
        case tNEQ:
        case tGT:
        case tGE:
        case tLT:
        case tLE:
            res[0] = res[1] = res[2] = I | D | S;
            res[3] = 1; // Only compare same types
            break;
        default:
            fprintf(stderr, "unknown operation %d\n", op);
            assert(false);

    }

    return res;
}


// correct types on stack to make them
// 1) equal
// 2) correspond resTypes
//
// @n -- 1 or 2 -- number of arguments
void BytecodeVisitor::correctTypes(int n, std::vector<uint8_t> resTypes)
{
    assert(1 <= n && n <= 2);

#define BAD_TYPE(id, type) (((resTypes[id] & (1 << type)) == 0) && (resTypes[id] == 1))

    if (n == 1) {
        VarType argType = types.top();

        if (BAD_TYPE(1, argType)) {
            fprintf(stderr, "can not correct type %d to types %d", argType, resTypes[1]);
            assert(false);
        }

        if ((resTypes[1] & (1 << argType)) == 0)
            convertType(VT_INT); // conver to int by default
    } else {
        VarType rhs = types.top();
        types.pop();
        VarType lhs = types.top();
        types.push(rhs);

        if (BAD_TYPE(1, lhs) || BAD_TYPE(2, rhs)) {
            fprintf(stderr, "can not correct type %d to types %d", lhs, resTypes[1]);
            fprintf(stderr, "can not correct type %d to types %d", rhs, resTypes[2]);
            assert(false);
        }

        if (lhs == rhs)
            return;

        VarType finalType = VT_DOUBLE;
        if (rhs == VT_STRING || lhs == VT_STRING)
            finalType = VT_INT;

        if (rhs != finalType)
            convertType(finalType);
        if (lhs != finalType) {
            types.pop();
            types.pop();
            types.push(rhs);
            types.push(lhs);
            _fun->bytecode()->addInsn(BC_SWAP);
            convertType(finalType);
        }
    }

#undef BAD_TYPE
}

void BytecodeVisitor::binaryMathOp(TokenKind op)
{
    correctTypes(2, opResType(op));
    VarType resType = types.top();

    switch (op) {
        case tAAND:
            _fun->bytecode()->addInsn(BC_IAAND);
            break;
        case tAOR:
            _fun->bytecode()->addInsn(BC_IAOR);
            break;
        case tAXOR:
            _fun->bytecode()->addInsn(BC_IAXOR);
            break;
        case tADD:
            if (resType == VT_INT)
                _fun->bytecode()->addInsn(BC_IADD);
            else
                _fun->bytecode()->addInsn(BC_DADD);
            break;
        case tSUB:
            if (resType == VT_INT)
                _fun->bytecode()->addInsn(BC_ISUB);
            else
                _fun->bytecode()->addInsn(BC_DSUB);
            break;
        case tMUL:
            if (resType == VT_INT)
                _fun->bytecode()->addInsn(BC_IMUL);
            else
                _fun->bytecode()->addInsn(BC_DMUL);
            break;
        case tDIV:
            if (resType == VT_INT)
                _fun->bytecode()->addInsn(BC_IDIV);
            else
                _fun->bytecode()->addInsn(BC_DDIV);
            break;
        case tMOD:
            _fun->bytecode()->addInsn(BC_IMOD);
            break;
        default:
            fprintf(stderr, "operator '%s' is not a valid binary math operator\n", tokenOp(op));
            assert(false);
    }
}

void BytecodeVisitor::visitBinaryOpNode(BinaryOpNode *node)
{
    node->left()->visit(this);
    node->right()->visit(this);

    TokenKind op = node->kind();

    switch (op) {
        default:
            binaryMathOp(op);
    }
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
    convertType(var->type());
    types.pop();

    // use STORE*VAR if var belongs to the current scope, STORECTX otherwise
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
