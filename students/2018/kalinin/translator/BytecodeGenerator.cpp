//
// Created by Владислав Калинин on 09/11/2018.
//

#include "BytecodeGenerator.h"
#include "TypeEvaluter.h"

using namespace mathvm;

void BytecodeGenerator::visitFunctionNode(mathvm::FunctionNode *node) {
    if (node->name() == "<top>") {
        node->body()->visit(new TypeEvaluter(ctx));
        node->body()->visit(this);
        bytecode->addInsn(BC_STOP);
    } else {
        for (int32_t i = node->parametersNumber() - 1; i >= 0; --i) {
            translateStoreVariable(node->parameterName(i), node->parameterType(i));
        }
        node->body()->visit(this);
    }
}

void BytecodeGenerator::visitIfNode(IfNode *node) {
    Label elseLabel(bytecode);
    Label endLabel(bytecode);
    node->ifExpr()->visit(this);
    bytecode->addInsn(BC_ILOAD0);
    bytecode->addBranch(BC_IFICMPNE, elseLabel);

    node->thenBlock()->visit(this);

    if (node->elseBlock() != nullptr) {
        bytecode->addBranch(BC_JA, endLabel);
        bytecode->bind(elseLabel);
        node->elseBlock()->visit(this);
        bytecode->bind(endLabel);
    } else {
        bytecode->bind(elseLabel);
    }
}

void BytecodeGenerator::visitWhileNode(WhileNode *node) {
    Label loopLabel(bytecode);
    Label endLabel(bytecode);

    bytecode->bind(loopLabel);
    node->whileExpr()->visit(this);
    bytecode->addInsn(BC_ILOAD0);
    bytecode->addBranch(BC_IFICMPNE, endLabel);
    node->loopBlock()->visit(this);
    bytecode->addBranch(BC_JA, loopLabel);
    bytecode->bind(endLabel);
}

void BytecodeGenerator::visitForNode(ForNode *node) {
    Label loopLabel(bytecode);
    Label endLabel(bytecode);
    auto *rangeNode = node->inExpr()->asBinaryOpNode();
    const AstVar *var = node->var();

    rangeNode->left()->visit(this);
    translateStoreVariable(var->name(), var->type());
    bytecode->bind(loopLabel);
    translateLoadVariable(var->name(), var->type());
    rangeNode->right()->visit(this);
    bytecode->addBranch(BC_IFICMPLE, endLabel);
    node->body()->visit(this);
    bytecode->addInsn(BC_ILOAD1);
    translateLoadVariable(var->name(), var->type());
    bytecode->addInsn(BC_IADD);
    translateStoreVariable(var->name(), var->type());
    bytecode->addBranch(BC_JA, loopLabel);
    bytecode->bind(endLabel);
}

Bytecode *BytecodeGenerator::getBytecode() {
    return bytecode;
}

void BytecodeGenerator::visitBlockNode(BlockNode *node) {
    ctx->nextSubContext();
    translateFunctionsBody(node->scope());
    for (uint32_t i = 0; i < node->nodes(); ++i) {
        AstNode *children = node->nodeAt(i);
        children->visit(this);
        if (isExpressionNode(children)) {
            bytecode->addInsn(BC_POP);
        }
    }
    ctx->nextSubContext();
}

void BytecodeGenerator::visitIntLiteralNode(IntLiteralNode *node) {
    bytecode->addInsn(BC_ILOAD);
    bytecode->addInt64(node->literal());
}

void BytecodeGenerator::visitDoubleLiteralNode(DoubleLiteralNode *node) {
    bytecode->addInsn(BC_DLOAD);
    bytecode->addDouble(node->literal());
}

void BytecodeGenerator::visitStringLiteralNode(StringLiteralNode *node) {
    bytecode->addInsn(BC_SLOAD);
    bytecode->addInt16(ctx->makeStringConstant(node->literal()));
}

void BytecodeGenerator::visitLoadNode(LoadNode *node) {
    node->visitChildren(this);
    translateLoadVariable(node->var()->name(), node->var()->type());
}

void BytecodeGenerator::visitStoreNode(StoreNode *node) {
    TokenKind op = node->op();
    const AstVar *var = node->var();
    if (op == tINCRSET || op == tDECRSET) {
        translateLoadVariable(var->name(), var->type());
        node->value()->visit(this);
        translateCastTypes(getType(node->value()), var->type());
        if (op == tINCRSET) {
            if (var->type() == VT_INT) {
                bytecode->addInsn(BC_IADD);
            } else {
                bytecode->addInsn(BC_DADD);
            }
        } else {
            if (var->type() == VT_INT) {
                bytecode->addInsn(BC_ISUB);
            } else {
                bytecode->addInsn(BC_DSUB);
            }
        }
    } else {
        node->value()->visit(this);
        translateCastTypes(getType(node->value()), var->type());
    }
    translateStoreVariable(var->name(), var->type());
}

void BytecodeGenerator::visitReturnNode(ReturnNode *node) {
    if (node->returnExpr() != nullptr) {
        node->returnExpr()->visit(this);
        translateCastTypes(getType(node->returnExpr()), getType(node));
    }
    bytecode->addInsn(BC_RETURN);
}

void BytecodeGenerator::visitBinaryOpNode(BinaryOpNode *node) {
    TokenKind op = node->kind();
    switch (op) {
        case tOR:
        case tAND:
            translateLogicOperation(node, op);
            break;
        case tAOR:
        case tAAND:
        case tAXOR:
            translateBitwiseOperation(node, op);
            break;
        case tMOD:
            node->visitChildren(this);
            bytecode->addInsn(BC_IMOD);
            break;
        case tEQ:
        case tNEQ:
        case tGT:
        case tGE:
        case tLT:
        case tLE:
            translateCompareOperation(node->left(), node->right(), op);
            break;
        case tADD:
        case tSUB:
        case tMUL:
        case tDIV:
            translateArithmeticOperation(node, op);
            break;
        default:
            break;
    }
}

void BytecodeGenerator::visitUnaryOpNode(UnaryOpNode *node) {
    TokenKind op = node->kind();
    if (op == tSUB) {
        translateNegateNumber(node);
    } else {
        translateInverseBoolean(node);
    }
}

void BytecodeGenerator::visitCallNode(CallNode *node) {
    auto *func = ctx->getFunction(node->name());
    for (uint32_t i = 0; i < node->parametersNumber(); ++i) {
        node->parameterAt(i)->visit(this);
        translateCastTypes(getType(node->parameterAt(i)), func->parameterType(i));
    }
    bytecode->addInsn(BC_CALL);
    bytecode->addInt16(func->id());
}

void BytecodeGenerator::visitPrintNode(PrintNode *node) {
    for (uint32_t i = 0; i < node->operands(); ++i) {
        node->operandAt(i)->visit(this);
        VarType operandType = getType(node->operandAt(i));
        if (operandType == VT_INT) {
            bytecode->addInsn(BC_IPRINT);
        } else if (operandType == VT_DOUBLE) {
            bytecode->addInsn(BC_DPRINT);
        } else {
            bytecode->addInsn(BC_SPRINT);
        }
    }
}

void BytecodeGenerator::translateBitwiseOperation(BinaryOpNode *node, TokenKind op) {
    node->visitChildren(this);
    switch (op) {
        case tAOR:
            bytecode->addInsn(BC_IAOR);
            break;
        case tAAND:
            bytecode->addInsn(BC_IAAND);
            break;
        case tAXOR:
            bytecode->addInsn(BC_IAXOR);
            break;
        default:
            break;
    }
}

void BytecodeGenerator::translateLogicAtom(Instruction compareInsn, Instruction trueResult,
                                           Label* endLabel) {
    Label falseLabel(bytecode);
    bytecode->addInsn(BC_ILOAD0);
    bytecode->addBranch(compareInsn, falseLabel);
    bytecode->addInsn(trueResult);
    bytecode->addBranch(BC_JA, *endLabel);
    bytecode->bind(falseLabel);
}


void BytecodeGenerator::translateLogicOperation(BinaryOpNode *node, TokenKind op) {
    Label firstIsFalseLabel(bytecode);
    Label endLabel(bytecode);

    Instruction compareInsn, trueResult, falseResult;

    if (op == tAND) {
        compareInsn = BC_IFICMPE;
        trueResult = BC_ILOAD0;
        falseResult = BC_ILOAD1;
    } else {
        compareInsn = BC_IFICMPNE;
        trueResult = BC_ILOAD1;
        falseResult = BC_ILOAD0;
    }

    node->left()->visit(this);
    translateLogicAtom(compareInsn, trueResult, &endLabel);
    node->right()->visit(this);
    translateLogicAtom(compareInsn, trueResult, &endLabel);
    bytecode->addInsn(falseResult);
    bytecode->bind(endLabel);
}


void BytecodeGenerator::translateCompareOperation(AstNode *left, AstNode *right, TokenKind op) {
    VarType leftType = getType(left);
    VarType rightType = getType(right);

    if (leftType == VT_DOUBLE || rightType == VT_DOUBLE) {
        left->visit(this);
        translateCastTypes(leftType, VT_DOUBLE);
        right->visit(this);
        translateCastTypes(rightType, VT_DOUBLE);
        bytecode->addInsn(BC_DCMP);
        bytecode->addInsn(BC_ILOAD0);
    } else {
        left->visit(this);
        translateCastTypes(leftType, VT_INT);
        right->visit(this);
        translateCastTypes(leftType, VT_INT);
    }

    Label elseLabel(bytecode);
    Label endLabel(bytecode);
    switch (op) {
        case tEQ:
            bytecode->addBranch(BC_IFICMPE, elseLabel);
            break;
        case tNEQ:
            bytecode->addBranch(BC_IFICMPNE, elseLabel);
            break;
        case tGT:
            bytecode->addBranch(BC_IFICMPG, elseLabel);
            break;
        case tGE:
            bytecode->addBranch(BC_IFICMPGE, elseLabel);
            break;
        case tLT:
            bytecode->addBranch(BC_IFICMPL, elseLabel);
            break;
        case tLE:
            bytecode->addBranch(BC_IFICMPLE, elseLabel);
            break;
        default:
            break;
    }
    bytecode->addInsn(BC_ILOAD1);
    bytecode->addBranch(BC_JA, endLabel);
    bytecode->bind(elseLabel);
    bytecode->addInsn(BC_ILOAD0);
    bytecode->bind(endLabel);
}

VarType BytecodeGenerator::getType(AstNode *node) {
    return *((VarType *) node->info());
}

void BytecodeGenerator::translateArithmeticOperation(BinaryOpNode *node, TokenKind op) {
    AstNode *left = node->left();
    AstNode *right = node->right();
    VarType leftType = getType(left);
    VarType rightType = getType(right);
    if (leftType == VT_DOUBLE || rightType == VT_DOUBLE) {
        left->visit(this);
        translateCastTypes(leftType, VT_DOUBLE);
        right->visit(this);
        translateCastTypes(rightType, VT_DOUBLE);
        switch (op) {
            case tADD:
                bytecode->addInsn(BC_DADD);
                break;
            case tSUB:
                bytecode->addInsn(BC_DSUB);
                break;
            case tMUL:
                bytecode->addInsn(BC_DMUL);
                break;
            case tDIV:
                bytecode->addInsn(BC_DDIV);
                break;
            default:
                break;
        }
    } else {
        node->visitChildren(this);
        switch (op) {
            case tADD:
                bytecode->addInsn(BC_IADD);
                break;
            case tSUB:
                bytecode->addInsn(BC_ISUB);
                break;
            case tMUL:
                bytecode->addInsn(BC_IMUL);
                break;
            case tDIV:
                bytecode->addInsn(BC_IDIV);
                break;
            default:
                break;
        }
    }
}

void BytecodeGenerator::translateNegateNumber(UnaryOpNode *node) {
    AstNode *operand = node->operand();
    VarType operandType = getType(operand);
    operand->visit(this);
    if (operandType == VT_DOUBLE) {
        bytecode->addInsn(BC_DNEG);
    } else {
        bytecode->addInsn(BC_INEG);
    }
}

void BytecodeGenerator::translateInverseBoolean(UnaryOpNode *node) {
    node->operand()->visit(this);

    Label elseLabel(bytecode);
    Label endLabel(bytecode);
    bytecode->addInsn(BC_ILOAD0);
    bytecode->addBranch(BC_IFICMPNE, elseLabel);
    bytecode->addInsn(BC_ILOAD0);
    bytecode->addBranch(BC_JA, endLabel);
    bytecode->bind(elseLabel);
    bytecode->addInsn(BC_ILOAD1);
    bytecode->bind(endLabel);
}

void BytecodeGenerator::translateLoadVariable(string varName, VarType varType) {
    Context *varContext = ctx->subContext()->getVarContext(varName);
    uint16_t varId = varContext->getVarId(varName);
    bool varInCurrentCtx = varContext->getId() == ctx->getId();
    if (varInCurrentCtx) {
        switch (varType) {
            case VT_INT:
                bytecode->addInsn(BC_LOADIVAR);
                break;
            case VT_DOUBLE:
                bytecode->addInsn(BC_LOADDVAR);
                break;
            case VT_STRING:
                bytecode->addInsn(BC_LOADSVAR);
                break;
            default:
                break;
        }
    } else {
        switch (varType) {
            case VT_INT:
                bytecode->addInsn(BC_LOADCTXIVAR);
                break;
            case VT_DOUBLE:
                bytecode->addInsn(BC_LOADCTXDVAR);
                break;
            case VT_STRING:
                bytecode->addInsn(BC_LOADCTXSVAR);
                break;
            default:
                break;
        }
        bytecode->addInt16(varContext->getId());
    }
    bytecode->addInt16(varId);
}

void BytecodeGenerator::translateStoreVariable(string varName, VarType varType) {
    Context *varContext = ctx->subContext()->getVarContext(varName);
    uint16_t varId = varContext->getVarId(varName);
    bool varInCurrentCtx = varContext->getId() == ctx->getId();
    if (varInCurrentCtx) {
        switch (varType) {
            case VT_INT:
                bytecode->addInsn(BC_STOREIVAR);
                break;
            case VT_DOUBLE:
                bytecode->addInsn(BC_STOREDVAR);
                break;
            case VT_STRING:
                bytecode->addInsn(BC_STORESVAR);
                break;
            default:
                break;
        }
    } else {
        switch (varType) {
            case VT_INT:
                bytecode->addInsn(BC_STORECTXIVAR);
                break;
            case VT_DOUBLE:
                bytecode->addInsn(BC_STORECTXDVAR);
                break;
            case VT_STRING:
                bytecode->addInsn(BC_STORECTXSVAR);
                break;
            default:
                break;
        }
        bytecode->addInt16(varContext->getId());
    }
    bytecode->addInt16(varId);
}

void BytecodeGenerator::translateCastTypes(VarType sourse, VarType target) {
    if (sourse == target) {
        return;
    }
    if (sourse == VT_INT && target == VT_DOUBLE) {
        bytecode->addInsn(BC_I2D);
    } else if (sourse == VT_STRING && target == VT_INT) {
        bytecode->addInsn(BC_S2I);
    } else if (sourse == VT_DOUBLE && target == VT_INT) {
        bytecode->addInsn(BC_D2I);
    }
}

bool BytecodeGenerator::isExpressionNode(AstNode *node) {
    if (node->isCallNode()) {
        return getType(node) != VT_VOID;
    }
    return node->isBinaryOpNode() || node->isUnaryOpNode() || node->isDoubleLiteralNode() || node->isIntLiteralNode()
           || node->isStringLiteralNode() || node->isLoadNode();
}

void BytecodeGenerator::translateFunctionsBody(Scope *scope) {
    Scope::FunctionIterator functionIterator(scope);
    Bytecode *currentBytecode = bytecode;
    Context *currentContext = ctx;
    int childNumber = 0;
    while (functionIterator.hasNext()) {
        auto *func = functionIterator.next();
        bytecode = currentContext->getFunction(func->name())->bytecode();
        ctx = currentContext->getChildAt(childNumber);
        func->node()->visit(this);
        ctx->destroySubContext();
        childNumber++;

        //TODO for debug
//        cout << "============[ " << func->name() << " ]============" << endl;
//        bytecode->dump(cout);
//        cout << "========================" << endl;
    }
    ctx = currentContext;
    bytecode = currentBytecode;
}