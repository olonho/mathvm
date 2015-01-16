#include "translatorVisitor.h"

//#define LOG(x) std::cout << x << "\n"; std::cout.flush();
#define LOG(x)

TranslatorVisitor::TranslatorVisitor(BytecodeScope *scope, BytecodeFunction *function)
        : currentScope(scope), currentFunction(function) {
}

void TranslatorVisitor::addId(idType id) {
//    std::cout << "adding id " << id << "\n";
    bytecode()->addUInt16(id);
}

void TranslatorVisitor::addDouble(double literal) {
    addInstruction(BC_DLOAD);
//    std::cout << "adding double " << literal << "\n";
    bytecode()->addDouble(literal);
}
void TranslatorVisitor::addInt(int64_t literal) {
    addInstruction(BC_ILOAD);
//    std::cout << "adding int " << literal << "\n";
    bytecode()->addInt64(literal);
}

void TranslatorVisitor::addInstruction(Instruction instruction) {
//    std::cout << "adding instruction " << bytecodeName(instruction, 0) << "\n";
    bytecode()->addInsn(instruction);
}

Bytecode *TranslatorVisitor::bytecode() {
    return currentFunction->bytecode();
}

void TranslatorVisitor::storeVariable(VarType variableType, idPair variableId) {
    if (variableId.first != currentScope->id) {
        //this is variable from other context
        switch (variableType) {
            case VT_INT:
                addInstruction(BC_STORECTXIVAR);
                break;
            case VT_DOUBLE:
                addInstruction(BC_STORECTXDVAR);
                break;
            case VT_STRING:
                addInstruction(BC_STORECTXSVAR);
                break;
            default:
                throw std::logic_error("Can't store non-value variable.");
        }
        addId(variableId.first - 1);
        addId(variableId.second);
    } else {
        switch (variableType) {
            case VT_INT:
                addInstruction(BC_STOREIVAR);
                break;
            case VT_DOUBLE:
                addInstruction(BC_STOREDVAR);
                break;
            case VT_STRING:
                addInstruction(BC_STORESVAR);
                break;
            default:
                throw std::logic_error("Can't store non-value variable.");
        }
        addId(variableId.second);
    }
}

void TranslatorVisitor::loadVariable(idPair variableId) {
    VarType variableType = currentScope->getVariable(variableId)->type();
    if (variableId.first != currentScope->id) {
        //this is variable from other context
        switch (variableType) {
            case VT_INT:
                addInstruction(BC_LOADCTXIVAR);
                break;
            case VT_DOUBLE:
                addInstruction(BC_LOADCTXDVAR);
                break;
            case VT_STRING:
                addInstruction(BC_LOADCTXSVAR);
                break;
            default:
                throw std::logic_error("Can't load non-value variable.");
        }
        addId(variableId.first - 1);
        addId(variableId.second);
    } else {
        switch (variableType) {
            case VT_INT:
                addInstruction(BC_LOADIVAR);
                break;
            case VT_DOUBLE:
                addInstruction(BC_LOADDVAR);
                break;
            case VT_STRING:
                addInstruction(BC_LOADSVAR);
                break;
            default:
                throw std::logic_error("Can't load non-value variable.");
        }
        addId(variableId.second);
    }
    tosType = variableType;
}


void TranslatorVisitor::castTo(VarType type) {
    if (type != tosType) {
        switch (type) {
            case VT_INT:
                castToInt();
                return;
            case VT_DOUBLE:
                castToDouble();
                return;
            default:
                throw std::logic_error("Can't cast to non-numeric type");
        }
    }
}

void TranslatorVisitor::castToDouble() {
    switch (tosType) {
        case VT_INT:
            addInstruction(BC_I2D);
            break;
        case VT_STRING:
            addInstruction(BC_S2I);
            addInstruction(BC_I2D);
            break;
        case VT_DOUBLE:
            break;
        default:
            throw std::logic_error("Can't cast to non-numeric type");
    }
    tosType = VT_DOUBLE;
}

void TranslatorVisitor::castToInt() {
    switch (tosType) {
        case VT_INT:
            break;
        case VT_STRING:
            addInstruction(BC_S2I);
            break;
        case VT_DOUBLE:
            addInstruction(BC_D2I);
            break;
        default:
            throw std::logic_error("Can't cast to non-numeric type");
    }
    tosType = VT_INT;
}


void TranslatorVisitor::convertBinOpArgs(VarType &leftType, VarType &rightType) {
    if (leftType != rightType) {
        if (leftType == VT_INT && rightType == VT_DOUBLE) {
            addSwap();
            intToDouble();
            addSwap();
            tosType = VT_DOUBLE;
            leftType = VT_DOUBLE;
        } else if (leftType == VT_DOUBLE && rightType == VT_INT) {
            intToDouble();
            tosType = VT_DOUBLE;
            rightType = VT_DOUBLE;
        }
    }
}

void TranslatorVisitor::intToDouble() {
    addInstruction(BC_I2D);
}

void TranslatorVisitor::addBinOpBytecode(TokenKind tokenKind, VarType leftType, VarType rightType) {
    assert(leftType == rightType);
    switch (tokenKind) {
        case tOR:
        case tAOR:
            addInstruction(BC_IAOR);
            tosType = VT_INT;
            return;
        case tAND:
        case tAAND:
            addInstruction(BC_IAAND);
            tosType = VT_INT;
            return;
        case tAXOR:
            addInstruction(BC_IAXOR);
            tosType = VT_INT;
            return;
        case tMOD:
            addInstruction(BC_IMOD);
            tosType = VT_INT;
            return;
        case tEQ:
        case tNEQ: {
            addInstruction(leftType == VT_INT ? BC_ICMP : BC_DCMP);
            tosType = VT_INT;
            if (tokenKind == tEQ) addInstruction(BC_INEG);
            return;
        }
        case tLT:
        case tGT: {
            addInstruction(leftType == VT_INT ? BC_ICMP : BC_DCMP);
            addInstruction(tokenKind == tGT ? BC_ILOAD1 : BC_ILOADM1);
            tosType = VT_INT;
            addInstruction(BC_ICMP);
            addInstruction(BC_INEG);
            return;
        }
        case tLE:
        case tGE: {
            addInstruction(leftType == VT_INT ? BC_ICMP : BC_DCMP);
            addInstruction(tokenKind == tGE ? BC_ILOADM1 : BC_ILOAD1);
            addInstruction(BC_ICMP);
            tosType = VT_INT;
            return;
        }
        case tADD:
            addInstruction(leftType == VT_INT ? BC_IADD : BC_DADD);
            tosType = leftType;
            return;
        case tSUB:
            addInstruction(leftType == VT_INT ? BC_ISUB : BC_DSUB);
            tosType = leftType;
            return;
        case tMUL:
            addInstruction(leftType == VT_INT ? BC_IMUL : BC_DMUL);
            tosType = leftType;
            return;
        case tDIV:
            addInstruction(leftType == VT_INT ? BC_IDIV : BC_DDIV);
            tosType = leftType;
            return;
        default:
            throw std::logic_error("Unsupported bin op");

    }
}

void TranslatorVisitor::visitBinaryOpNode(BinaryOpNode *node) {
    LOG("visiting binOp")
    node->left()->visit(this);
    VarType leftType = tosType;
    node->right()->visit(this);
    VarType rightType = tosType;
    assert(leftType == VT_INT || leftType == VT_DOUBLE);
    assert(rightType == VT_INT || rightType == VT_DOUBLE);
    convertAndAddBinOp(node->kind(), leftType, rightType);
}

void TranslatorVisitor::convertAndAddBinOp(TokenKind opKind, VarType leftType, VarType rightType) {
    assert(leftType == VT_INT || leftType == VT_DOUBLE);
    assert(rightType == VT_INT || rightType == VT_DOUBLE);
    switch (opKind) {
        case tOR:
        case tAND:
        case tAOR:
        case tAAND:
        case tAXOR:
        case tMOD: {
            assert(leftType == VT_INT);
            assert(rightType == VT_INT);
            addBinOpBytecode(opKind, leftType, rightType);
            break;
        }
        case tEQ:
        case tNEQ:
        case tGT:
        case tGE:
        case tLT:
        case tLE:
        case tADD:
        case tSUB:
        case tMUL:
        case tDIV: {
            convertBinOpArgs(leftType, rightType);
            addBinOpBytecode(opKind, leftType, rightType);
            break;
        }
        default:
            throw std::logic_error("Unsupported bin op");
    }
}


void TranslatorVisitor::clearStack(AstNode *node) {
    bool pushesValueToStack =
            node->isBinaryOpNode() ||
                    node->isUnaryOpNode() ||
                    node->isStringLiteralNode() ||
                    node->isDoubleLiteralNode() ||
                    node->isIntLiteralNode() ||
                    node->isLoadNode();
    if (!pushesValueToStack && node->isCallNode()) {
        std::string const &functionName = node->asCallNode()->name();
        BytecodeFunction *function = currentScope->getFunction(functionName);
        pushesValueToStack = function && function->returnType() != VT_VOID;
    }
    if (pushesValueToStack) {
        addInstruction(BC_POP);
    }
}


void TranslatorVisitor::visitBlockNode(BlockNode *node) {
    LOG("visiting BlockNode")
    Scope *nodeScope = node->scope();
    //add variables
    for (Scope::VarIterator varIt(nodeScope); varIt.hasNext();) {
        AstVar *variable = varIt.next();
        currentScope->registerVariable(variable->type(), variable->name());
    }
    //add functions
    for (Scope::FunctionIterator funIt(nodeScope); funIt.hasNext();) {
        AstFunction *function = funIt.next();
        currentScope->registerFunction(new BytecodeFunction(function));
    }
    for (uint32_t i = 0; i < node->nodes(); ++i) {
        AstNode *element = node->nodeAt(i);
        element->visit(this);
        clearStack(element);
    }
    //generate bytecode for functions
    for (Scope::FunctionIterator funIt(nodeScope); funIt.hasNext();) {
        visitFunctionNode(funIt.next()->node());
    }
}

void TranslatorVisitor::visitCallNode(CallNode *node) {
    LOG("visiting callNode")
    BytecodeFunction *bytecodeFunction = currentScope->getFunction(node->name());
    idType functionId = bytecodeFunction != NULL ? bytecodeFunction->id() : INVALID_ID;
    if (functionId == INVALID_ID) {
        throw std::logic_error("Unknown function id");
    }
    for (uint32_t i = node->parametersNumber(); i > 0; --i) {
        //calculate parameters on tos
        node->parameterAt(i - 1)->visit(this);
        //cast params if necessary
        castTo(bytecodeFunction->parameterType(i - 1));
    }
    addInstruction(BC_CALL);
    addId(functionId);
    if (bytecodeFunction->returnType() != VT_VOID) {
        tosType = bytecodeFunction->returnType();
    }
}

void TranslatorVisitor::visitDoubleLiteralNode(DoubleLiteralNode *node) {
    LOG("visiting doubleLiteral")
    addDouble(node->literal());
    tosType = VT_DOUBLE;
}

void TranslatorVisitor::branch(Instruction instruction, Label &label) {
    bytecode()->addBranch(instruction, label);
}

void TranslatorVisitor::visitForNode(ForNode *node) {
    LOG("visiting forNode")
    AstVar const *loopVariable = node->var();
    idPair loopVarAndScopeIds = currentScope->getVariableAndScope(loopVariable->name());
    if (loopVarAndScopeIds.first == INVALID_ID || loopVarAndScopeIds.second) {
        throw std::logic_error("Failed to detect variable and scope id.");
    }

    BinaryOpNode const *inExpr = dynamic_cast<BinaryOpNode const *>(node->inExpr());
    if (inExpr->kind() != tRANGE) {
        throw std::logic_error("No range in for loop detected.");
    }

    //init loop variable
    inExpr->left()->visit(this);
    castToInt(); //in case the expr happened to be double
    addInstruction(BC_ILOAD1);
    addInstruction(BC_ISUB);
    castTo(loopVariable->type());
    storeVariable(loopVariable->type(), loopVarAndScopeIds);

    Label loop(bytecode());
    Label breakLoop(bytecode());
    bytecode()->bind(loop);

    //calculate new loop variable value
    loadVariable(loopVarAndScopeIds);
    castToInt();
    addInstruction(BC_ILOAD1);
    addInstruction(BC_IADD);
    castTo(loopVariable->type());
    storeVariable(loopVariable->type(), loopVarAndScopeIds);

    //check loop condition
    inExpr->right()->visit(this);
    castToInt();
    loadVariable(loopVarAndScopeIds);
    castToInt();
    branch(BC_IFICMPG, breakLoop);

    //execute loop body
    node->body()->visit(this);
    branch(BC_JA, loop);

    bytecode()->bind(breakLoop);
}

void TranslatorVisitor::visitFunctionNode(FunctionNode *node) {
    LOG("visiting functionNode")
    BytecodeScope *functionScope = currentScope->createChild();
    BytecodeFunction *function = currentScope->getFunction(node->name());
    TranslatorVisitor visitor(functionScope, function);

    for (uint32_t i = 0; i != node->parametersNumber(); ++i) {
        uint16_t variableId = functionScope->registerVariable(node->parameterType(i), node->parameterName(i));
        visitor.storeVariable(node->parameterType(i), std::make_pair(functionScope->id, variableId));
    }
    visitor.visitBlockNode(node->body());
    function->setLocalsNumber(functionScope->varsCount());
}

void TranslatorVisitor::visitIfNode(IfNode *node) {
    LOG("visiting ifNode")
    node->ifExpr()->visit(this);
    castToInt();
    addInstruction(BC_ILOAD0);

    Label afterIfNode(bytecode());
    if (node->elseBlock()) {
        Label thenBlock(bytecode());
        branch(BC_IFICMPE, thenBlock);
        node->thenBlock()->visit(this);
        branch(BC_JA, afterIfNode);
        bytecode()->bind(thenBlock);
        node->elseBlock()->visit(this);
    } else {
        branch(BC_IFICMPE, afterIfNode);
        node->thenBlock()->visit(this);
    }
    bytecode()->bind(afterIfNode);
}

void TranslatorVisitor::visitIntLiteralNode(IntLiteralNode *node) {
    LOG("visiting intLiteral")
    addInt(node->literal());
    tosType = VT_INT;
}

void TranslatorVisitor::visitLoadNode(LoadNode *node) {
    LOG("visiting loadNode")
    AstVar const * variable = node->var();

    idPair scopeAndVariableId = currentScope->getVariableAndScope(variable->name());

    if (scopeAndVariableId.first == INVALID_ID || scopeAndVariableId.second == INVALID_ID) {
        throw std::logic_error("Failed to detect variable and scope id.");
    }

    loadVariable(scopeAndVariableId);
}

void TranslatorVisitor::visitNativeCallNode(NativeCallNode *node) {
    throw std::logic_error("Native calls are not supported yet.");
}

void TranslatorVisitor::visitPrintNode(PrintNode *node) {
    LOG("visiting printNode")
    for (uint32_t i = 0; i < node->operands(); ++i) {
        node->operandAt(i)->visit(this);
        switch (tosType) {
            case VT_STRING: addInstruction(BC_SPRINT); break;
            case VT_INT: addInstruction(BC_IPRINT); break;
            case VT_DOUBLE: addInstruction(BC_DPRINT); break;
            default: throw std::logic_error("Can only print value-typed expressions.");
        }
    }
}

void TranslatorVisitor::visitReturnNode(ReturnNode *node) {
    LOG("visiting returnNode")
    if (node->returnExpr() != NULL) {
        node->returnExpr()->visit(this);
        castTo(currentFunction->returnType());
    }
    addInstruction(BC_RETURN);
}

void TranslatorVisitor::visitStoreNode(StoreNode *node) {
    LOG("visiting storeNode")
    node->value()->visit(this);
    VarType variableType = node->var()->type();
    idPair scopeAndVariableId = currentScope->getVariableAndScope(node->var()->name());
    if (scopeAndVariableId.first == INVALID_ID || scopeAndVariableId.second == INVALID_ID) {
        throw std::logic_error("Failed to detect variable and scope id.");
    }
    if (node->op() != tASSIGN) {
        //it's += or -=
        loadVariable(scopeAndVariableId);
        addSwap();
        //convertAndAddBinOp(node->op() == tDECRSET ? tSUB : tADD, variableType, tosType);
        convertAndAddBinOp(node->op() == tINCRSET ? tADD : tSUB, variableType, tosType);
    }
    castTo(variableType);
    storeVariable(variableType, scopeAndVariableId);
}

void TranslatorVisitor::visitStringLiteralNode(StringLiteralNode *node) {
    LOG("visiting stringLiteralNode")

    idType stringConstantId = currentScope->getStringConstant(node->literal());
    addInstruction(BC_SLOAD);
    addId(stringConstantId);
    tosType = VT_STRING;
}

void TranslatorVisitor::visitUnaryOpNode(UnaryOpNode *node) {
    LOG("visiting unaryOpNode")
    node->visitChildren(this);
    switch (node->kind()) {
        case tNOT:
            switch (tosType) {
                case VT_INT:
                    addInstruction(BC_INEG);
                    break;
                case VT_DOUBLE:
                    addInstruction(BC_DNEG);
                    break;
                default:
                    throw std::logic_error("Can't negate non-numeric values.");
            }
            break;
        case tSUB:
            switch (tosType) {
                case VT_INT:
                    addInstruction(BC_ILOAD0);
                    addSwap();
                    addInstruction(BC_ISUB);
                    break;
                case VT_DOUBLE:
                    addInstruction(BC_DLOAD0);
                    addSwap();
                    addInstruction(BC_DSUB);
                    break;
                default:
                    throw std::logic_error("Can't negate non-numeric values.");
            }
            break;
        default:
            throw std::logic_error("Unknown unary operation.");
    }
}

void TranslatorVisitor::visitWhileNode(WhileNode *node) {
    LOG("visiting whileNode")
    Label breakLoop(bytecode());
    Label loop(bytecode());

    bytecode()->bind(loop);
    node->whileExpr()->visit(this);
    castToInt();
    addInstruction(BC_ILOAD0);
    branch(BC_IFICMPE, breakLoop);
    node->loopBlock()->visit(this);
    branch(BC_JA, loop);

    bytecode()->bind(breakLoop);
}

void TranslatorVisitor::addSwap() {
    addInstruction(BC_SWAP);
}


