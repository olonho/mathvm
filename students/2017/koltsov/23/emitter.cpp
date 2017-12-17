#include "parser.h"
#include "interpreter_code.h"

#include "emitter.h"


#include <dlfcn.h>
#include <bits/stdc++.h>
using namespace std;

namespace mathvm {


void BytecodeEmitterVisitor::setCode(Code *code) {
    _code = dynamic_cast<InterpreterCodeImpl*>(code);
}

void BytecodeEmitterVisitor::emitFromTop(AstFunction* top) {
    addFunction(top);

    while (not _functions.empty()) {
        auto front = _functions.front();
        _functions.pop();
        enterFunction(front);

        for (uint32_t i = 0; i < currentFunction->parametersNumber(); ++i) {
            switch (currentFunction->parameterType(i)) {
                case VT_INT:
                    bytecode()->addInsn(BC_STORECTXIVAR);
                    break;
                case VT_DOUBLE:
                    bytecode()->addInsn(BC_STORECTXDVAR);
                    break;
                case VT_STRING:
                    bytecode()->addInsn(BC_STORECTXSVAR);
                    break;

                default:
                    my_error("unknown func param type");
            }
            bytecode()->addTyped(currentFunction->scopeId());
            bytecode()->addTyped(getVarId(
                        byteToAstFunc[currentFunction]->scope()
                            ->lookupVariable(currentFunction->parameterName(i))));
        } 
        
        byteToAstFunc[front]->node()->visit(this);
        if (byteToAstFunc[front] == top) {
            bytecode()->addInsn(BC_STOP);
        }
        exitFunction();
    }
}


VarType BytecodeEmitterVisitor::popType() {
    auto tp = _typeStack.top();
    _typeStack.pop();
    return tp;
}

#define setNeedPop() \
needPop = true;  \
DBG(__FUNCTION__)

#define unsetNeedPop() \
needPop = false;  \

uint16_t BytecodeEmitterVisitor::getVarId(const AstVar* var) {
    if (varToId.count(var) == 0) {
        varToId[var] = varToId.size();
    }

    return varToId[var];
}

uint16_t BytecodeEmitterVisitor::getScopeId(Scope* scope) {
    if (scopeToId.count(scope) == 0) {
        scopeToId[scope] = scopeToId.size();
        DBG(scopeToId[scope]);
        DBG(scope);
    }

    return scopeToId[scope];
}

Bytecode* BytecodeEmitterVisitor::bytecode() {
    return currentFunction->bytecode();
}

void BytecodeEmitterVisitor::addFunction(AstFunction* func) {
    auto bcFunc = new BytecodeFunction(func);
    byteToAstFunc[bcFunc] = func;
    bcFunc->setScopeId(getScopeId(func->scope()));
    _functions.push(bcFunc);
    _code->addFunction(bcFunc);
}

void BytecodeEmitterVisitor::enterFunction(BytecodeFunction* func) {
    currentFunction = func;
}

void BytecodeEmitterVisitor::exitFunction() {
    currentFunction = nullptr;
}

bool isGeneralIntegralOp(TokenKind kind) {
    return kind == tADD or
        kind == tSUB or
        kind == tMUL or 
        kind == tDIV;
}

void BytecodeEmitterVisitor::addInstrsByKind(TokenKind kind) {
    switch (kind) {
        // Only doubles
        case tADD: 
            bytecode()->addInsn(BC_DADD);
            break;
        case tSUB: 
            bytecode()->addInsn(BC_DSUB);
            break;
        case tMUL: 
            bytecode()->addInsn(BC_DMUL);
            break;
        case tDIV:
            bytecode()->addInsn(BC_DDIV);
            break;

        // 0/1 ints or plain ints
        case tOR:  
        case tAOR:
            bytecode()->addInsn(BC_IAOR);
            break;
        case tAND:
        case tAAND:
            bytecode()->addInsn(BC_IAAND);
            break;

        // plain ints
        case tAXOR:
            bytecode()->addInsn(BC_IAXOR);
            break;
        case tMOD:
            bytecode()->addInsn(BC_IMOD);
            break;


        case tRANGE:
            // nop
            break;

        default:
            my_error("unknonwn TokenKind");
    }
}

void BytecodeEmitterVisitor::castTOS(VarType from, VarType to) {
    if (from == to)
        return;

    if (to == VT_INT) {
        if (from == VT_DOUBLE)
            bytecode()->addInsn(BC_D2I);
        else
            bytecode()->addInsn(BC_S2I);
    } else if (to == VT_DOUBLE and from == VT_INT) {
        bytecode()->addInsn(BC_I2D);
    } else {
        my_error("unknown cast");
    }
}

void BytecodeEmitterVisitor::convertTOSIntToBoolean(ConversionOp op=NEQ_ZERO) {
    Label end(bytecode()), one(bytecode());

    Instruction insn;
    switch (op) {
        case GT_ZERO: 
            insn = BC_IFICMPL; 
            break;
        case NEQ_ZERO:
            insn = BC_IFICMPNE;
            break;
        case LT_ZERO:
            insn = BC_IFICMPG;
            break;
        default:
            my_error("bad enum");
    }

    bytecode()->addInsn(BC_ILOAD0);
    bytecode()->addBranch(insn, one);
    bytecode()->addInsn(BC_ILOAD0);
    bytecode()->addBranch(BC_JA, end);

    bytecode()->bind(one);
    bytecode()->addInsn(BC_ILOAD1);
    bytecode()->bind(end);
}

void BytecodeEmitterVisitor::doLogicalNot() {
    convertTOSIntToBoolean();
    // now 0 or 1 => !x = 1 - x
    bytecode()->addInsn(BC_ILOAD1);
    bytecode()->addInsn(BC_ISUB);
}

void BytecodeEmitterVisitor::doComparison(TokenKind kind) {
    switch (kind) {
        case tEQ:
            doComparison(tNEQ);
            doLogicalNot();
            return;
        case tGE:
            doComparison(tLT);
            doLogicalNot();
            return;
        case tLE:
            doComparison(tGT);
            doLogicalNot();
            return;
        case tNEQ:
            // a != b <=> cmp(a, b) != 0 <=> bool(cmp(a, b)) == 1
            bytecode()->addInsn(BC_ICMP);
            convertTOSIntToBoolean();
            break;
        case tGT:
            // a > b <=> cmp(a, b) == 1 <=> cmp(a, b) > 0
            bytecode()->addInsn(BC_ICMP);
            convertTOSIntToBoolean(GT_ZERO);
            break;
        case tLT:
            // a > b <=> cmp(a, b) == 1 <=> cmp(a, b) < 0
            bytecode()->addInsn(BC_ICMP);
            convertTOSIntToBoolean(LT_ZERO);
            break;

        default:
            my_error("invalid comparison");
    }
}

void BytecodeEmitterVisitor::visitBinaryOpNode(BinaryOpNode* node) {
    node->right()->visit(this);
    node->left()->visit(this);

    auto tupper = popType();
    auto tlower = popType();

    auto kind = node->kind();

    auto castTo = VT_INT;
    if (isGeneralIntegralOp(kind)) {
        castTo = VT_DOUBLE;
    } 
    bytecode()->addInsn(BC_SWAP);
    castTOS(tlower, castTo);
    bytecode()->addInsn(BC_SWAP);
    castTOS(tupper, castTo);

    if (tEQ <= kind and kind <= tLE) {
        doComparison(kind);
        _typeStack.push(VT_INT);
        return;
    }

    // Logical, hard.
    if (kind == tOR or kind == tAND) {
        bytecode()->addInsn(BC_SWAP);
        convertTOSIntToBoolean();
        bytecode()->addInsn(BC_SWAP);
        convertTOSIntToBoolean();
    }

    addInstrsByKind(kind);
    _typeStack.push(castTo);
    setNeedPop();
}

void BytecodeEmitterVisitor::visitUnaryOpNode(UnaryOpNode* node) {
    node->operand()->visit(this);
    auto type = popType();

    switch (node->kind()) {
        case tNOT:
            doLogicalNot();
            _typeStack.push(VT_INT);
            break;
        case tSUB:
            if (type == VT_INT) {
                bytecode()->addInsn(BC_INEG);
            } else if (type == VT_DOUBLE) {
                bytecode()->addInsn(BC_DNEG);
            } else {
                my_error("cannot negate string");
            }

            _typeStack.push(type);
            break;
        default:
            my_error("unknown unop");
    }

    setNeedPop();
}       

void BytecodeEmitterVisitor::visitStringLiteralNode(StringLiteralNode* node) {
    bytecode()->addInsn(BC_SLOAD);
    bytecode()->addTyped(_code->makeStringConstant(node->literal()));
    _typeStack.push(VT_STRING);
    setNeedPop();
}

void BytecodeEmitterVisitor::visitDoubleLiteralNode(DoubleLiteralNode* node) {
    bytecode()->addInsn(BC_DLOAD);
    bytecode()->addTyped(node->literal());
    _typeStack.push(VT_DOUBLE);
    setNeedPop();
}

void BytecodeEmitterVisitor::visitIntLiteralNode(IntLiteralNode* node) {
    bytecode()->addInsn(BC_ILOAD);
    bytecode()->addTyped(node->literal());
    _typeStack.push(VT_INT);
    setNeedPop();
}

void BytecodeEmitterVisitor::loadVar(const AstVar* var) {
    auto scopeId = getScopeId(var->owner());
    auto varId = getVarId(var);

    switch (var->type()) {
        case VT_DOUBLE:
            bytecode()->addInsn(BC_LOADCTXDVAR); 
            break;
        case VT_INT:
            bytecode()->addInsn(BC_LOADCTXIVAR); 
            break;
        case VT_STRING:
            bytecode()->addInsn(BC_LOADCTXSVAR);
            break;
        default:
            my_error("Invalid value type to load");
    }

    bytecode()->addTyped(scopeId);
    bytecode()->addTyped(varId);
}

void BytecodeEmitterVisitor::visitLoadNode(LoadNode* node) {
    loadVar(node->var());
    _typeStack.push(node->var()->type());

    setNeedPop();
}

void BytecodeEmitterVisitor::visitStoreNode(StoreNode* node) {
    node->value()->visit(this);
    unsetNeedPop();

    auto type = popType();
    castTOS(type, node->var()->type());

    auto varId = getVarId(node->var());
    auto scopeId = getScopeId(node->var()->owner());

    Instruction insn;
    switch (node->var()->type()) {
        case VT_DOUBLE:
            insn = BC_STORECTXDVAR; 
            break;
        case VT_INT:
            insn = BC_STORECTXIVAR; 
            break;
        case VT_STRING:
            insn = BC_STORECTXSVAR;
            break;
        default:
            my_error("Invalid value type to store");
    }


    switch (node->op()) {
        case tASSIGN:
            break;
        case tINCRSET:
            loadVar(node->var());
            if (node->var()->type() == VT_INT)
                bytecode()->addInsn(BC_IADD);
            else
                bytecode()->addInsn(BC_DADD);

            break;
        case tDECRSET:
            loadVar(node->var());
            if (node->var()->type() == VT_INT)
                bytecode()->addInsn(BC_ISUB);
            else
                bytecode()->addInsn(BC_DSUB);
            break;
        default:
            my_error("unknown store operation");
    }

    bytecode()->addInsn(insn);
    bytecode()->addTyped(scopeId);
    bytecode()->addTyped(varId);
}

void BytecodeEmitterVisitor::visitForNode(ForNode* node) {
    auto in = (BinaryOpNode*)node->inExpr();

    Label startIf(bytecode()), afterLoop(bytecode());

    // i = low
    in->left()->visit(this);
    unsetNeedPop();
    bytecode()->addInsn(BC_STORECTXIVAR);
    bytecode()->addTyped(getScopeId(node->var()->owner()));
    bytecode()->addTyped(getVarId(node->var()));

    // if (i <= high)
    // { 
    //  body
    //  i++
    // }
    bytecode()->bind(startIf);
    loadVar(node->var());
    in->right()->visit(this);
    unsetNeedPop();
    popType();
    bytecode()->addBranch(BC_IFICMPL, afterLoop);
    node->body()->visit(this);
    loadVar(node->var());
    bytecode()->addInsn(BC_ILOAD1);
    bytecode()->addInsn(BC_IADD);
    bytecode()->addInsn(BC_STORECTXIVAR);
    bytecode()->addTyped(getScopeId(node->var()->owner()));
    bytecode()->addTyped(getVarId(node->var()));
    bytecode()->addBranch(BC_JA, startIf);

    bytecode()->bind(afterLoop);
}

void BytecodeEmitterVisitor::visitWhileNode(WhileNode* node) {
    Label startIf(bytecode()), afterLoop(bytecode());

    bytecode()->bind(startIf);
    node->whileExpr()->visit(this);
    unsetNeedPop();
    popType();
    bytecode()->addInsn(BC_ILOAD1);
    bytecode()->addBranch(BC_IFICMPNE, afterLoop);
    node->loopBlock()->visit(this);
    bytecode()->addBranch(BC_JA, startIf);

    bytecode()->bind(afterLoop); 
}

void BytecodeEmitterVisitor::visitIfNode(IfNode* node) {
    Label startElse(bytecode()), afterIf(bytecode());

    node->ifExpr()->visit(this);
    unsetNeedPop();
    convertTOSIntToBoolean();
    bytecode()->addInsn(BC_ILOAD1);
    popType();
    bytecode()->addBranch(BC_IFICMPNE, startElse);
    node->thenBlock()->visit(this);
    bytecode()->addBranch(BC_JA, afterIf);

    bytecode()->bind(startElse);
    if (node->elseBlock()) {
        node->elseBlock()->visit(this);
    }

    bytecode()->bind(afterIf);
}

void BytecodeEmitterVisitor::visitBlockNode(BlockNode* node) {
    scopeToId[node->scope()] = getScopeId(byteToAstFunc[currentFunction]->scope());
    Scope::FunctionIterator funIter(node->scope());
    while (funIter.hasNext()) {
        AstFunction* fun = funIter.next();
        addFunction(fun);
    }

    needPop = false;
    for (uint32_t i = 0; i < node->nodes(); ++i) {
        node->nodeAt(i)->visit(this);

        if (needPop) {
            popType();
            bytecode()->addInsn(BC_POP);
            needPop = false;
        }
    }
}

void BytecodeEmitterVisitor::visitFunctionNode(FunctionNode* node) {
    node->body()->visit(this);
}

void BytecodeEmitterVisitor::visitReturnNode(ReturnNode* node) {
    if (node->returnExpr()) {
        node->returnExpr()->visit(this);
        castTOS(popType(), currentFunction->returnType());
        _typeStack.push(currentFunction->returnType());
    }
    bytecode()->addInsn(BC_RETURN);
}

void BytecodeEmitterVisitor::visitCallNode(CallNode* node) {
    auto func = _code->functionByName(node->name());

    for (uint32_t i = node->parametersNumber(); i > 0; --i) {
        node->parameterAt(i - 1)->visit(this);
        castTOS(popType(), func->parameterType(i - 1));
    }
    unsetNeedPop();

    bytecode()->addInsn(BC_CALL);
    bytecode()->addTyped(func->id());

    if (func->returnType() != VT_VOID) {
        setNeedPop();
        _typeStack.push(func->returnType());
    }
}

void BytecodeEmitterVisitor::visitNativeCallNode(NativeCallNode* node) {
    auto pnt = dlsym(RTLD_DEFAULT, node->nativeName().c_str());
    auto id = _code->makeNativeFunction(
            node->nativeName(), 
            node->nativeSignature(), 
            pnt);

    bytecode()->addInsn(BC_CALLNATIVE);
    bytecode()->addTyped(id);
}

void BytecodeEmitterVisitor::visitPrintNode(PrintNode* node) {
    for (uint32_t i = 0; i < node->operands(); i++) {
        node->operandAt(i)->visit(this);
        unsetNeedPop();
        switch (popType()) {
            case VT_INT:
                bytecode()->addInsn(BC_IPRINT);
                break;
            case VT_DOUBLE:
                bytecode()->addInsn(BC_DPRINT);
                break;
            case VT_STRING:
                bytecode()->addInsn(BC_SPRINT);
                break;
            default:
                my_error("nothing to print");
        }
    }
}

} // namespace mathvm
