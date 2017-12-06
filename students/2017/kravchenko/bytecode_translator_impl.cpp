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
    printf("registering function %s\n", a_fun->name().c_str());
    _funcs.push_back(a_fun);
    BytecodeFunction *fun = new BytecodeFunction(a_fun);
    _code->addFunction(fun);
    _funIdMap[a_fun->name()] = fun->id();

    for (Scope::FunctionIterator fun_it(a_fun->node()->body()->scope()); fun_it.hasNext();) {
        AstFunction *fun = fun_it.next();
        registerFunctions(fun);
    }
}

void BytecodeVisitor::translate(AstFunction *a_fun)
{
    fprintf(stderr, "in b_visitor\n");
    // get functions and scopes ids.
    registerScopes(a_fun->owner());
    registerFunctions(a_fun);

    for (AstFunction *fun : _funcs) {
        _fun = nullptr;
        _scope = nullptr;
        assert(types.size() == 0);
        assert(_scopeSizes.size() == 0);
        translateAstFunction(fun);
    }

    printf("\n\n\n");
}

void BytecodeVisitor::addInsn(Instruction insn)
{
    _fun->bytecode()->addInsn(insn);
    switch (insn) {
// ints
        case BC_ILOAD:
        case BC_ILOAD0:
        case BC_ILOAD1:
        case BC_ILOADM1:
            types.push(VT_INT);
            break;

        case BC_IADD:
        case BC_ISUB:
        case BC_IMUL:
        case BC_IDIV:
        case BC_IMOD:
        case BC_IAOR:
        case BC_IAAND:
        case BC_IAXOR:
        case BC_ICMP:
            assert(types.top() == VT_INT);
            types.pop();
            assert(types.top() == VT_INT);
            types.pop();
            types.push(VT_INT);
            break;

        case BC_INEG:
            assert(types.top() == VT_INT);
            break;

        case BC_IPRINT:
            assert(types.top() == VT_INT);
            types.pop();
            break;

        case BC_LOADIVAR:
        case BC_LOADIVAR0:
        case BC_LOADIVAR1:
        case BC_LOADIVAR2:
        case BC_LOADIVAR3:
        case BC_LOADCTXIVAR:
            types.push(VT_INT);
            break;

        case BC_STOREIVAR:
        case BC_STOREIVAR0:
        case BC_STOREIVAR1:
        case BC_STOREIVAR2:
        case BC_STOREIVAR3:
        case BC_STORECTXIVAR:
            assert(types.top() == VT_INT);
            types.pop();
            break;

        case BC_IFICMPNE:
        case BC_IFICMPE:
        case BC_IFICMPG:
        case BC_IFICMPGE:
        case BC_IFICMPL:
        case BC_IFICMPLE:
            assert(types.top() == VT_INT);
            types.pop();
            assert(types.top() == VT_INT);
            types.push(VT_INT);
            break;

// doubles
        case BC_DLOAD:
        case BC_DLOAD0:
        case BC_DLOAD1:
        case BC_DLOADM1:
            types.push(VT_DOUBLE);
            break;

        case BC_DADD:
        case BC_DSUB:
        case BC_DMUL:
        case BC_DDIV:
            assert(types.top() == VT_DOUBLE);
            types.pop();
            assert(types.top() == VT_DOUBLE);
            types.pop();
            types.push(VT_DOUBLE);
            break;

        case BC_DCMP:
            assert(types.top() == VT_DOUBLE);
            types.pop();
            assert(types.top() == VT_DOUBLE);
            types.pop();
            types.push(VT_INT);
            break;

        case BC_DNEG:
            assert(types.top() == VT_DOUBLE);
            break;

        case BC_DPRINT:
            assert(types.top() == VT_DOUBLE);
            types.pop();
            break;

        case BC_LOADDVAR:
        case BC_LOADDVAR0:
        case BC_LOADDVAR1:
        case BC_LOADDVAR2:
        case BC_LOADDVAR3:
        case BC_LOADCTXDVAR:
            types.push(VT_DOUBLE);
            break;

        case BC_STOREDVAR:
        case BC_STOREDVAR0:
        case BC_STOREDVAR1:
        case BC_STOREDVAR2:
        case BC_STOREDVAR3:
        case BC_STORECTXDVAR:
            assert(types.top() == VT_DOUBLE);
            types.pop();
            break;

// strings
        case BC_SLOAD:
        case BC_SLOAD0:
            types.push(VT_STRING);
            break;

        case BC_SPRINT:
            assert(types.top() == VT_STRING);
            types.pop();
            break;

        case BC_LOADSVAR:
        case BC_LOADSVAR0:
        case BC_LOADSVAR1:
        case BC_LOADSVAR2:
        case BC_LOADSVAR3:
        case BC_LOADCTXSVAR:
            types.push(VT_STRING);
            break;

        case BC_STORESVAR:
        case BC_STORESVAR0:
        case BC_STORESVAR1:
        case BC_STORESVAR2:
        case BC_STORESVAR3:
        case BC_STORECTXSVAR:
            assert(types.top() == VT_STRING);
            types.pop();
            break;

// casts
        case BC_I2D:
            assert(types.top() == VT_INT);
            types.pop();
            types.push(VT_DOUBLE);
            break;
        case BC_D2I:
            assert(types.top() == VT_DOUBLE);
            types.pop();
            types.push(VT_INT);
            break;
        case BC_S2I:
            assert(types.top() == VT_STRING);
            types.pop();
            types.push(VT_INT);
            break;

// any
        case BC_SWAP:
            {
                VarType t1 = types.top();
                types.pop();
                VarType t2 = types.top();
                types.pop();
                types.push(t1);
                types.push(t2);
                break;
            }
        case BC_CALL: // takes return address
            assert(types.top() == VT_INT);
        case BC_POP:
            types.pop();
            break;

// not modifying stack
        case BC_JA:
        case BC_STOP:
        case BC_BREAK:
        case BC_CALLNATIVE:
        case BC_RETURN: // special case
            break;

        default:
            fprintf(stderr, "unknown instruction %d\n", insn);
            assert(false);
            break;
    }
}

void BytecodeVisitor::translateAstFunction(AstFunction *a_fun)
{
    _fun = (BytecodeFunction *)_code->functionByName(a_fun->name());
    _fun->setScopeId(_scope_map[a_fun->scope()]);
    _fun->setLocalsNumber(a_fun->node()->body()->scope()->variablesCount());

    _scope = a_fun->scope();

    a_fun->node()->visit(this);

    fprintf(stderr, "translated ast function %s:\n", _fun->name().c_str());
    fprintf(stderr, "    id = %d\n", _fun->id());
    fprintf(stderr, "    locals = %d\n", _fun->localsNumber());
    fprintf(stderr, "    params = %d\n", _fun->parametersNumber());
    fprintf(stderr, "    scopeId = %d\n", _fun->scopeId());
    _fun->bytecode()->dump(std::cout);
}

void BytecodeVisitor::convertType(VarType to)
{
    VarType from = types.top();

    if (from == to)
        return;
    if (from == VT_INT && to == VT_DOUBLE) {
        addInsn(BC_I2D);
        return;
    }
    if (from == VT_DOUBLE && to == VT_INT) {
        addInsn(BC_D2I);
        return;
    }
    if (from == VT_STRING && to == VT_INT) {
        addInsn(BC_S2I);
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
        case tRANGE:
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
            addInsn(BC_SWAP);
            convertType(finalType);
        }
    }

#undef BAD_TYPE
}

void BytecodeVisitor::binaryMathOp(BinaryOpNode *node)
{
    node->left()->visit(this);
    node->right()->visit(this);

    TokenKind op = node->kind();

    correctTypes(2, opResType(op));
    VarType resType = types.top();

    switch (op) {
        case tAAND:
            addInsn(BC_IAAND);
            break;
        case tAOR:
            addInsn(BC_IAOR);
            break;
        case tAXOR:
            addInsn(BC_IAXOR);
            break;
        case tADD:
            if (resType == VT_INT)
                addInsn(BC_IADD);
            else
                addInsn(BC_DADD);
            break;
        case tSUB:
            if (resType == VT_INT)
                addInsn(BC_ISUB);
            else
                addInsn(BC_DSUB);
            break;
        case tMUL:
            if (resType == VT_INT)
                addInsn(BC_IMUL);
            else
                addInsn(BC_DMUL);
            break;
        case tDIV:
            if (resType == VT_INT)
                addInsn(BC_IDIV);
            else
                addInsn(BC_DDIV);
            break;
        case tMOD:
            addInsn(BC_IMOD);
            break;
        default:
            fprintf(stderr, "operator '%s' is not a valid binary math operator\n", tokenOp(op));
            assert(false);
    }
}

void BytecodeVisitor::binaryCompareOp(BinaryOpNode *node)
{
    node->left()->visit(this);
    node->right()->visit(this);

    TokenKind op = node->kind();

    correctTypes(2, opResType(op));
    VarType operandsType = types.top();

    if (operandsType == VT_STRING) {
        addInsn(BC_S2I);
        addInsn(BC_SWAP);
        addInsn(BC_S2I);
        addInsn(BC_SWAP);
        operandsType = VT_INT;
    }

    if (operandsType == VT_INT)
        addInsn(BC_ICMP);
    else
        addInsn(BC_DCMP);
    addInsn(BC_ILOAD0);

    Label thn(_fun->bytecode());
    Label els(_fun->bytecode());

    switch (op) {
        case tEQ:
            _fun->bytecode()->addBranch(BC_IFICMPE,  thn);
            break;
        case tNEQ:
            _fun->bytecode()->addBranch(BC_IFICMPNE, thn);
            break;
        case tGT:
            _fun->bytecode()->addBranch(BC_IFICMPG, thn);
            break;
        case tGE:
            _fun->bytecode()->addBranch(BC_IFICMPGE, thn);
            break;
        case tLT:
            _fun->bytecode()->addBranch(BC_IFICMPL, thn);
            break;
        case tLE:
            _fun->bytecode()->addBranch(BC_IFICMPLE, thn);
            break;

        default:
            fprintf(stderr, "operation %s is not a compare operation\n", tokenOp(op));
            assert(false);
    }

    addInsn(BC_POP);
    addInsn(BC_POP);
    addInsn(BC_ILOAD0);
    _fun->bytecode()->addBranch(BC_JA, els);
    _fun->bytecode()->bind(thn);

    types.pop();
    types.push(VT_INT);
    types.push(VT_INT);

    addInsn(BC_POP);
    addInsn(BC_POP);
    addInsn(BC_ILOAD1);
    _fun->bytecode()->bind(els);
}

void BytecodeVisitor::binaryLogicOp(BinaryOpNode *node)
{
    TokenKind op = node->kind();
    if (op == tOR) {
        Label fail(_fun->bytecode());
        Label succ(_fun->bytecode());

        node->left()->visit(this);
        VarType lhs = types.top();
        if (lhs == VT_STRING) {
            addInsn(BC_S2I);
            lhs = VT_INT;
        }

        if (lhs == VT_INT) {
            addInsn(BC_ILOAD0);
            _fun->bytecode()->addBranch(BC_IFICMPNE, succ);
        } else {
            addInsn(BC_DLOAD0);
            addInsn(BC_DCMP);
            addInsn(BC_ILOAD0);
            _fun->bytecode()->addBranch(BC_IFICMPNE, succ);
        }

        addInsn(BC_POP);
        addInsn(BC_POP);

        node->right()->visit(this);
        VarType rhs = types.top();
        if (rhs == VT_STRING) {
            addInsn(BC_S2I);
            rhs = VT_INT;
        }

        if (rhs == VT_INT) {
            addInsn(BC_ILOAD0);
            _fun->bytecode()->addBranch(BC_IFICMPNE, succ);
        } else {
            addInsn(BC_DLOAD0);
            addInsn(BC_DCMP);
            addInsn(BC_ILOAD0);
            _fun->bytecode()->addBranch(BC_IFICMPNE, succ);
        }

        addInsn(BC_POP);
        addInsn(BC_POP);

        addInsn(BC_ILOAD0);
        _fun->bytecode()->addBranch(BC_JA, fail);
        _fun->bytecode()->bind(succ);

        types.pop();
        types.push(VT_INVALID);
        types.push(VT_INVALID);

        addInsn(BC_POP);
        addInsn(BC_POP);
        addInsn(BC_ILOAD1);
        _fun->bytecode()->bind(fail);

    } else {
        Label fail(_fun->bytecode());
        Label succ(_fun->bytecode());

        node->left()->visit(this);
        VarType lhs = types.top();
        if (lhs == VT_STRING) {
            addInsn(BC_S2I);
            lhs = VT_INT;
        }

        if (lhs == VT_INT) {
            addInsn(BC_ILOAD0);
            _fun->bytecode()->addBranch(BC_IFICMPE, fail);
        } else {
            addInsn(BC_DLOAD0);
            addInsn(BC_DCMP);
            addInsn(BC_ILOAD0);
            _fun->bytecode()->addBranch(BC_IFICMPE, fail);
        }

        addInsn(BC_POP);
        addInsn(BC_POP);

        node->right()->visit(this);
        VarType rhs = types.top();
        if (rhs == VT_STRING) {
            addInsn(BC_S2I);
            rhs = VT_INT;
        }

        if (rhs == VT_INT) {
            addInsn(BC_ILOAD0);
            _fun->bytecode()->addBranch(BC_IFICMPE, fail);
        } else {
            addInsn(BC_DLOAD0);
            addInsn(BC_DCMP);
            addInsn(BC_ILOAD0);
            _fun->bytecode()->addBranch(BC_IFICMPE, fail);
        }

        addInsn(BC_POP);
        addInsn(BC_POP);

        addInsn(BC_ILOAD1);
        _fun->bytecode()->addBranch(BC_JA, succ);
        _fun->bytecode()->bind(fail);

        types.pop();
        types.push(VT_INVALID);
        types.push(VT_INVALID);

        addInsn(BC_POP);
        addInsn(BC_POP);
        addInsn(BC_ILOAD0);
        _fun->bytecode()->bind(succ);
    }
}

void BytecodeVisitor::visitBinaryOpNode(BinaryOpNode *node)
{
    printf("visitBinaryOpNode '%s'\n", tokenOp(node->kind()));

    TokenKind op = node->kind();
    switch (op) {
        case tRANGE:
            node->left()->visit(this);
            node->right()->visit(this);
            correctTypes(2, opResType(op));
            break;
        case tEQ:
        case tNEQ:
        case tGT:
        case tGE:
        case tLT:
        case tLE:
            binaryCompareOp(node);
            break;
        case tOR:
        case tAND:
            binaryLogicOp(node);
            break;
        default:
            binaryMathOp(node);
            break;
    }
}

void BytecodeVisitor::visitUnaryOpNode(UnaryOpNode *node)
{
    printf("visitUnaryOpNode '%s'\n", tokenOp(node->kind()));

    node->operand()->visit(this);

    TokenKind op = node->kind();
    VarType resType = types.top();

    if (resType != VT_INT && resType != VT_DOUBLE) {
        fprintf(stderr, "unary operating %s can't be applied to type %d\n", tokenOp(op), resType);
        assert(false);
    }

    switch (op) {
        case tADD:
            break;
        case tSUB:
            if (resType == VT_INT)
                addInsn(BC_INEG);
            else
                addInsn(BC_DNEG);
        case tNOT:
            if (resType == VT_INT) {
                addInsn(BC_ILOAD0);
                addInsn(BC_ICMP);
                addInsn(BC_STOREIVAR0);
                addInsn(BC_LOADIVAR0);
                addInsn(BC_LOADIVAR0);
                addInsn(BC_IMUL);
            } else {
                addInsn(BC_DLOAD0);
                addInsn(BC_DCMP);
                addInsn(BC_STOREIVAR0);
                addInsn(BC_LOADIVAR0);
                addInsn(BC_LOADIVAR0);
                addInsn(BC_IMUL);
            }
            break;
        default:
            fprintf(stderr, "operating %s is not an unary operation\n", tokenOp(op));
            assert(false);
    }
}

void BytecodeVisitor::visitStringLiteralNode(StringLiteralNode *node)
{
    printf("visitStringLiteralNode '%s'\n", node->literal().c_str());

    uint16_t id = _code->makeStringConstant(node->literal());
    addInsn(BC_SLOAD);
    _fun->bytecode()->addUInt16(id);
}

void BytecodeVisitor::visitIntLiteralNode(IntLiteralNode *node)
{
    printf("visitIntLiteralNode '%ld'\n", node->literal());

    addInsn(BC_ILOAD);
    _fun->bytecode()->addInt64(node->literal());
}

void BytecodeVisitor::visitDoubleLiteralNode(DoubleLiteralNode *node)
{
    printf("visitDoubleLiteralNode '%f'\n", node->literal());

    addInsn(BC_DLOAD);
    _fun->bytecode()->addDouble(node->literal());
}

void BytecodeVisitor::visitLoadNode(LoadNode *node)
{
    printf("visitLoadNode var '%s'\n", node->var()->name().c_str());

    const AstVar *var = node->var();
    uint16_t scope_id = _scope_map[var->owner()];
    uint16_t var_id = _var_map[_scope][var->name()];

    if (var->type() == VT_INT) {
        if (scope_id == _scope_map[_scope])
            addInsn(BC_LOADIVAR);
        else
            addInsn(BC_LOADCTXIVAR);
    } else if (var->type() == VT_DOUBLE) {
        if (scope_id == _scope_map[_scope])
            addInsn(BC_LOADDVAR);
        else
            addInsn(BC_LOADCTXDVAR);
    } else if (var->type() == VT_STRING) {
        if (scope_id == _scope_map[_scope])
            addInsn(BC_LOADSVAR);
        else
            addInsn(BC_LOADCTXSVAR);
    }

    if (scope_id != _scope_map[_scope])
        _fun->bytecode()->addUInt16(scope_id);
    _fun->bytecode()->addUInt16(var_id);
}

void BytecodeVisitor::visitStoreNode(StoreNode *node)
{
    printf("visitStoreNode var '%s'\n", node->var()->name().c_str());

    const AstVar *var = node->var();
    uint16_t scope_id = _scope_map[var->owner()];
    uint16_t var_id = _var_map[_scope][var->name()];

    if (node->op() == tINCRSET || node->op() == tDECRSET) {
        LoadNode n(0, var);
        n.visit(this);
    }

    node->value()->visit(this);
    convertType(var->type());

    // use STORE*VAR if var belongs to the current scope, STORECTX otherwise
    if (var->type() == VT_INT) {
        if (node->op() == tINCRSET)
            addInsn(BC_IADD);
        if (node->op() == tDECRSET)
            addInsn(BC_ISUB);
        if (scope_id == _scope_map[_scope])
            addInsn(BC_STOREIVAR);
        else
            addInsn(BC_STORECTXIVAR);
    } else if (var->type() == VT_DOUBLE) {
        if (node->op() == tINCRSET)
            addInsn(BC_DADD);
        if (node->op() == tDECRSET)
            addInsn(BC_DSUB);
        if (scope_id == _scope_map[_scope])
            addInsn(BC_STOREDVAR);
        else
            addInsn(BC_STORECTXDVAR);
    } else if (var->type() == VT_STRING) {
        if (node->op() == tINCRSET)
            assert(false);
        if (node->op() == tDECRSET)
            assert(false);
        if (scope_id == _scope_map[_scope])
            addInsn(BC_STORESVAR);
        else
            addInsn(BC_STORECTXSVAR);
    }

    if (scope_id != _scope_map[_scope])
        _fun->bytecode()->addUInt16(scope_id);
    _fun->bytecode()->addUInt16(var_id);
}

void BytecodeVisitor::enterScope()
{
    _scopeSizes.push(types.size());
}

void BytecodeVisitor::leaveScope()
{
    size_t curSize = types.size();
    size_t prevSize = _scopeSizes.top();

    while (curSize != prevSize) {
        addInsn(BC_POP);
        curSize--;
    }

    _scopeSizes.pop();
}

void BytecodeVisitor::visitBlockNode(BlockNode *node)
{
    _scope = node->scope();

    printf("visiting block for scope %p, scope id = %d\n", _scope, _scope_map[_scope]);

    enterScope();

    for (int i = 0; i < (int)node->nodes(); i++) {
        AstNode *child = node->nodeAt(i);
        enterScope(); // hack
        child->visit(this);
        leaveScope(); // hack, pop unused values
        _scope = node->scope();
    }

    leaveScope();
}

void BytecodeVisitor::visitNativeCallNode(NativeCallNode *node)
{
}

void BytecodeVisitor::visitForNode(ForNode *node)
{
    node->inExpr()->visit(this);
    addInsn(BC_SWAP);

    assert(node->var()->type() == VT_INT);

    uint16_t scope_id = _scope_map[node->var()->owner()];
    uint16_t var_id = _var_map[node->var()->owner()][node->var()->name()];

    addInsn(BC_STORECTXIVAR);
    _fun->bytecode()->addUInt16(scope_id);
    _fun->bytecode()->addUInt16(var_id);

    Label begin = _fun->bytecode()->currentLabel();
    Label done(_fun->bytecode());
    _fun->bytecode()->addBranch(BC_IFICMPG, done);

    node->body()->visit(this);

    addInsn(BC_LOADCTXIVAR);
    _fun->bytecode()->addUInt16(scope_id);
    _fun->bytecode()->addUInt16(var_id);
    addInsn(BC_ILOAD1);
    addInsn(BC_IADD);
    addInsn(BC_STORECTXIVAR);
    _fun->bytecode()->addUInt16(scope_id);
    _fun->bytecode()->addUInt16(var_id);

    _fun->bytecode()->addBranch(BC_JA, begin);
    _fun->bytecode()->bind(done);
}

void BytecodeVisitor::visitWhileNode(WhileNode *node)
{
    Label repeat = _fun->bytecode()->currentLabel();

    node->whileExpr()->visit(this);

    VarType expType = types.top();
    if (expType == VT_DOUBLE)
        addInsn(BC_D2I);
    else if (expType == VT_STRING)
        addInsn(BC_S2I);

    addInsn(BC_ILOAD0);

    Label done(_fun->bytecode());

    _fun->bytecode()->addBranch(BC_IFICMPE, done);

    addInsn(BC_POP);
    addInsn(BC_POP);

    node->loopBlock()->visit(this);

    _fun->bytecode()->addBranch(BC_JA, repeat);
    _fun->bytecode()->bind(done);

    types.push(VT_INT);
    types.push(VT_INT);

    addInsn(BC_POP);
    addInsn(BC_POP);
}

void BytecodeVisitor::visitIfNode(IfNode *node)
{
    node->ifExpr()->visit(this);

    VarType expType = types.top();
    if (expType == VT_DOUBLE)
        addInsn(BC_D2I);
    else if (expType == VT_STRING)
        addInsn(BC_S2I);

    addInsn(BC_ILOAD0);

    Label notThen(_fun->bytecode());
    Label notElse(_fun->bytecode());

    _fun->bytecode()->addBranch(BC_IFICMPE, notThen);

    addInsn(BC_POP);
    addInsn(BC_POP);

    node->thenBlock()->visit(this);

    if (node->elseBlock())
        _fun->bytecode()->addBranch(BC_JA, notElse);

    _fun->bytecode()->bind(notThen);

    if (node->elseBlock()) {
        types.push(VT_INT);
        types.push(VT_INT);

        addInsn(BC_POP);
        addInsn(BC_POP);

        node->elseBlock()->visit(this);

        _fun->bytecode()->bind(notElse);
    }
}

void BytecodeVisitor::visitReturnNode(ReturnNode *node)
{
    printf("visitReturnNode\n");

    if (node->returnExpr())
        node->returnExpr()->visit(this);

    VarType returnType = types.top();

    assert(returnType == _fun->returnType());

    if (returnType == VT_INT)
        addInsn(BC_STOREIVAR0);
    else if (returnType == VT_DOUBLE)
        addInsn(BC_STOREDVAR0);
    else if (returnType == VT_STRING)
        addInsn(BC_STORESVAR0);

    int pops = (int)types.size() - 1; // leave only return address on stack
    for (int i = 0; i < pops; i++)
        _fun->bytecode()->addInsn(BC_POP);

    addInsn(BC_RETURN);
}

void BytecodeVisitor::visitFunctionNode(FunctionNode *node)
{
    // TODO natives
    printf("visiting function node name %s\n", node->name().c_str());

    // args
    for (int i = node->parametersNumber() - 1; i >= 0; i--) {
        VarType paramType = node->parameterType(i);
        if (paramType == VT_INT)
            types.push(VT_INT);
        else if (paramType == VT_DOUBLE)
            types.push(VT_DOUBLE);
        else if (paramType == VT_STRING)
            types.push(VT_STRING);
        else
            assert(false);
    }
    // return address
    types.push(VT_INT);

    for (size_t i = 0; i < node->parametersNumber(); i++) {
        VarType paramType = node->parameterType(i);
        addInsn(BC_SWAP); // return addres
        if (paramType == VT_INT)
            addInsn(BC_STOREIVAR);
        else if (paramType == VT_DOUBLE)
            addInsn(BC_STOREDVAR);
        else if (paramType == VT_STRING)
            addInsn(BC_STORESVAR);
        _fun->bytecode()->addUInt16(_var_map[_scope][node->parameterName(i)]);
    }

    node->body()->visit(this);

    // return address
    types.pop();

    printf("types size = %lu\n", types.size());
    if (_fun->bytecode()->getInsn(_fun->bytecode()->current() - 1) != BC_RETURN)
        addInsn(BC_RETURN); // fake return just in case
}

void BytecodeVisitor::visitCallNode(CallNode *node)
{
    enterScope(); // hack

    AstFunction *fun = _funcs[_funIdMap[node->name()]];
    assert(node->parametersNumber() == fun->parametersNumber());

    for (int i = node->parametersNumber() - 1, j = 0; i >= 0; i--) {
        node->parameterAt(i)->visit(this);
        convertType(fun->parameterType(j));
        j++;
    }

    addInsn(BC_ILOAD);
    _fun->bytecode()->addInt64(_fun->bytecode()->current() + 8 + 1 + 2);
    addInsn(BC_CALL);
    _fun->bytecode()->addUInt16(_funIdMap[node->name()]);

    leaveScope(); // hack

    VarType funType = fun->returnType();
    if (funType == VT_INT)
        addInsn(BC_LOADIVAR0);
    else if (funType == VT_DOUBLE)
        addInsn(BC_LOADDVAR0);
    else if (funType == VT_STRING)
        addInsn(BC_LOADSVAR0);
}

void BytecodeVisitor::visitPrintNode(PrintNode *node)
{
    for (int i = 0; i < (int)node->operands(); i++) {
        node->operandAt(i)->visit(this);
        VarType opType = types.top();
        if (opType == VT_INT)
            addInsn(BC_IPRINT);
        else if (opType == VT_DOUBLE)
            addInsn(BC_DPRINT);
        else if (opType == VT_STRING)
            addInsn(BC_SPRINT);
        else
            assert(false);
    }
}

}
