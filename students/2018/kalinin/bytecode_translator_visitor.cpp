//
// Created by Владислав Калинин on 09/11/2018.
//

#include "bytecode_translator_visitor.h"

using namespace mathvm;

void BytecodeTranslator::visitFunctionNode(mathvm::FunctionNode *node) {
    if (node->name() == "<top>") {
        node->body()->visit(new TypeEvaluter(ctx));
    }
    node->body()->visit(this);
}

void BytecodeTranslator::visitIfNode(IfNode *node) {
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

void BytecodeTranslator::visitWhileNode(WhileNode *node) {
    Label loopLabel(bytecode);
    Label endLabel(bytecode);

    bytecode->bind(loopLabel);
    node->whileExpr()->visit(this);
    bytecode->addInsn(BC_ILOAD0);
    bytecode->addBranch(BC_IFICMPE, endLabel);
    node->loopBlock()->visit(this);
    bytecode->addBranch(BC_JA, loopLabel);
    bytecode->bind(endLabel);
}

void BytecodeTranslator::visitForNode(ForNode *node) {
    Label loopLabel(bytecode);
    Label endLabel(bytecode);
    auto *rangeNode = node->inExpr()->asBinaryOpNode();

    rangeNode->left()->visit(this);
    translateStoreVariable(node->var());
    bytecode->bind(loopLabel);
    translateLoadVariable(node->var());
    rangeNode->right()->visit(this);
    bytecode->addBranch(BC_IFICMPL, endLabel);
    node->body()->visit(this);
    bytecode->addBranch(BC_JA, loopLabel);
    bytecode->bind(endLabel);
}

Bytecode *BytecodeTranslator::getBytecode() {
    return bytecode;
}

void BytecodeTranslator::visitBlockNode(BlockNode *node) {
    Context *currentCtx = ctx;
    ctx = currentCtx->childsIterator()->next();
    translateFunctionsBody(node->scope());
    for (uint32_t i = 0; i < node->nodes(); ++i) {
        AstNode *children = node->nodeAt(i);
        children->visit(this);
        if (isExpressionNode(children)) {
            bytecode->addInsn(BC_POP);
        }
    }
    ctx = currentCtx;
}

void BytecodeTranslator::visitIntLiteralNode(IntLiteralNode *node) {
    bytecode->addInsn(BC_ILOAD);
    bytecode->addInt64(node->literal());
}

void BytecodeTranslator::visitDoubleLiteralNode(DoubleLiteralNode *node) {
    bytecode->addInsn(BC_DLOAD);
    bytecode->addDouble(node->literal());
}

void BytecodeTranslator::visitStringLiteralNode(StringLiteralNode *node) {
    bytecode->addInsn(BC_SLOAD);
    bytecode->addInt16(ctx->makeStringConstant(node->literal()));
}

void BytecodeTranslator::visitLoadNode(LoadNode *node) {
    node->visitChildren(this);
    translateLoadVariable(node->var());
}

void BytecodeTranslator::visitStoreNode(StoreNode *node) {
    TokenKind op = node->op();
    const AstVar *var = node->var();
    if (op == tINCRSET || op == tDECRSET) {
        translateLoadVariable(var);
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
    translateStoreVariable(var);
}

void BytecodeTranslator::visitReturnNode(ReturnNode *node) {
    if (node->returnExpr() != nullptr) {
        node->returnExpr()->visit(this);
        translateCastTypes(getType(node->returnExpr()), getType(node));
    }
    bytecode->addInsn(BC_RETURN);
}

void BytecodeTranslator::visitBinaryOpNode(BinaryOpNode *node) {
    TokenKind op = node->kind();
    switch (op) {
        case tOR:
        case tAND:
        case tAOR:
        case tAAND:
        case tAXOR:
            translateBooleanOperation(node, op);
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

void BytecodeTranslator::visitUnaryOpNode(UnaryOpNode *node) {
    TokenKind op = node->kind();
    if (op == tSUB) {
        translateNegateNumber(node);
    } else {
        translateInverseBoolean(node);
    }
}

void BytecodeTranslator::visitCallNode(CallNode *node) {
    auto *func = ctx->getFunction(node->name());
    for (uint32_t i = 0; i < node->parametersNumber(); ++i) {
        node->parameterAt(i)->visit(this);
        translateCastTypes(getType(node->parameterAt(i)), func->parameterType(i));
    }
    bytecode->addInsn(BC_CALL);
    bytecode->addInt16(func->id());
}

void BytecodeTranslator::visitPrintNode(PrintNode *node) {
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

void BytecodeTranslator::translateBooleanOperation(BinaryOpNode *node, TokenKind op) {
    node->visitChildren(this);
    switch (op) {
        case tOR:
        case tAOR:
            bytecode->addInsn(BC_IAOR);
            break;
        case tAND:
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

void BytecodeTranslator::translateCompareOperation(AstNode *left, AstNode *right, TokenKind op) {
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

VarType BytecodeTranslator::getType(AstNode *node) {
    return *((VarType *) node->info());
}

void BytecodeTranslator::translateArithmeticOperation(BinaryOpNode *node, TokenKind op) {
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

void BytecodeTranslator::translateNegateNumber(UnaryOpNode *node) {
    AstNode *operand = node->operand();
    VarType operandType = getType(operand);
    operand->visit(this);
    if (operandType == VT_DOUBLE) {
        bytecode->addInsn(BC_DNEG);
    } else {
        bytecode->addInsn(BC_INEG);
    }
}

void BytecodeTranslator::translateInverseBoolean(UnaryOpNode *node) {
    node->operand()->visit(this);

    Label elseLabel(bytecode);
    Label endLabel(bytecode);
    bytecode->addInsn(BC_ILOAD0);
    bytecode->addBranch(BC_IFICMPNE, elseLabel);
    bytecode->addInsn(BC_ILOAD1);
    bytecode->addBranch(BC_JA, endLabel);
    bytecode->bind(elseLabel);
    bytecode->addInsn(BC_ILOAD0);
    bytecode->bind(endLabel);
}

void BytecodeTranslator::translateLoadVariable(const AstVar *var) {
    Context *varContext = ctx->getVarContext(var->name());
    uint16_t varId = varContext->getVarId(var->name());
    bool varInCurrentCtx = varContext == ctx;
    if (varInCurrentCtx) {
        switch (var->type()) {
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
        switch (var->type()) {
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

void BytecodeTranslator::translateStoreVariable(const AstVar *var) {
    Context *varContext = ctx->getVarContext(var->name());
    uint16_t varId = varContext->getVarId(var->name());
    bool varInCurrentCtx = varContext == ctx;
    if (varInCurrentCtx) {
        switch (var->type()) {
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
        switch (var->type()) {
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

void BytecodeTranslator::translateCastTypes(VarType sourse, VarType target) {
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

bool BytecodeTranslator::isExpressionNode(AstNode *node) {
    if (node->isCallNode()) {
        return getType(node) != VT_VOID;
    }
    return node->isBinaryOpNode() || node->isUnaryOpNode() || node->isDoubleLiteralNode() || node->isIntLiteralNode()
           || node->isStringLiteralNode() || node->isLoadNode();
}

void BytecodeTranslator::translateFunctionsBody(Scope *scope) {
    Scope::FunctionIterator functionIterator(scope);
    Bytecode *currentBytecode = bytecode;
    while (functionIterator.hasNext()) {
        auto *func = functionIterator.next();
        bytecode = ctx->getFunction(func->name())->bytecode();
        func->node()->visit(this);

        //TODO for debug
//        cout << "============[ " << func->name() << " ]============" << endl;
//        bytecode->dump(cout);
//        cout << "========================" << endl;
    }
    bytecode = currentBytecode;
}

//==============================[Context]==================================================================

void Context::addVar(Var *var) {
    varList.push_back(var);
    variablesById[var->name()] = static_cast<unsigned short>(varList.size() - 1);
}

void Context::addFun(AstFunction *func) {
    auto *byteCodeFunction = new BytecodeFunction(func);
    uint16_t functionId = static_cast<uint16_t>(functionList.size());
    byteCodeFunction->assignId(functionId);
    functionList.push_back(byteCodeFunction);
    functionsById[func->name()] = functionId;
}

Context *Context::instanse = nullptr;

vector<Context *> Context::contextList{};

vector<BytecodeFunction *> Context::functionList{};

Context *Context::getParentContext() {
    return parent;
}

uint16_t Context::getId() {
    return id;
}

Context *Context::getLastChildren() {
    return childs.back();
}

Context *Context::addChild() {
    auto *child = new Context(this);
    childs.push_back(child);
    return child;
}

uint16_t Context::VarNumber() {
    return static_cast<uint16_t>(variablesById.size());
}

BytecodeFunction *Context::getFunction(string name) {
    if (parent == nullptr) {
        return nullptr;
    }
    if (functionsById.find(name) != functionsById.end()) {
        return functionList[functionsById[name]];
    }
    return parent->getFunction(name);
}

Context *Context::getVarContext(string name) {
    if (parent == nullptr) {
        return nullptr;
    }
    if (variablesById.find(name) != variablesById.end()) {
        return this;
    }
    return parent->getVarContext(name);
}

uint16_t Context::getVarId(string name) {
    return variablesById[name];
}

Context *Context::getRoot() {
    if (instanse == nullptr) {
        instanse = new Context();
    }
    return instanse;
}

void Context::init(Context *parentContext) {
    parent = parentContext;
    id = static_cast<uint16_t>(contextList.size());
    iter = new ChildsIterator(&childs);
    contextList.push_back(this);
}

Context::ChildsIterator *Context::childsIterator() {
    return iter;
}

uint16_t Context::makeStringConstant(string literal) {
    uint16_t id = static_cast<unsigned short>(constantsById.size());
    constantsById[literal] = id;
    return id;
}

//===================================[TypeEvaluter]==========================================================

void TypeEvaluter::visitLoadNode(LoadNode *node) {
    setType(node, node->var()->type());
}

void TypeEvaluter::visitBinaryOpNode(BinaryOpNode *node) {
    node->visitChildren(this);
    VarType left = getType(node->left());
    VarType right = getType(node->right());
    TokenKind op = node->kind();
    if (left == VT_INVALID || left == VT_VOID || right == VT_INVALID || right == VT_VOID) {
        throw CompileError("MismatchTypeException", node->position());
    }
    VarType result = VT_INVALID;
    if (op == tOR || op == tAND || op == tAOR || op == tAAND || op == tAXOR || op == tMOD) {
        result = checkIntegerOperation(left, right);
    } else if (op == tGT || op == tGE || op == tLT || op == tLE) {
        result = checkCompareOperation(left, right);
    } else if (op == tEQ || op == tNEQ) {
        result = checkEqualsOperation(left, right);
    } else if (op == tSUB || op == tMUL || op == tDIV || op == tADD) {
        result = checkArithmeticOperation(left, right);
    } else if (op == tRANGE) {
        result = checkRangeOperation(left, right);
    }
    if (result == VT_INVALID) {
        throw CompileError("MismatchTypeException", node->position());
    }
    setType(node, result);
}

void TypeEvaluter::visitUnaryOpNode(UnaryOpNode *node) {
    node->visitChildren(this);
    VarType type = getType(node->operand());
    TokenKind op = node->kind();
    if (op == tSUB && (type == VT_INT || type == VT_DOUBLE)) {
        setType(node, type);
    } else if (op == tNOT && type == VT_INT) {
        setType(node, type);
    } else {
        throw CompileError("MismatchTypeException", node->position());
    }
}

void TypeEvaluter::visitIntLiteralNode(IntLiteralNode *node) {
    setType(node, VT_INT);
}

void TypeEvaluter::visitDoubleLiteralNode(DoubleLiteralNode *node) {
    setType(node, VT_DOUBLE);
}

void TypeEvaluter::visitStringLiteralNode(StringLiteralNode *node) {
    setType(node, VT_STRING);
}

void TypeEvaluter::visitFunctionNode(FunctionNode *node) {
    node->body()->visit(this);
    auto *function = ctx->getFunction(node->name());
    function->setLocalsNumber(ctx->VarNumber());
    function->setScopeId(ctx->getLastChildren()->getId());
    setType(node, VT_VOID);
}

void TypeEvaluter::fillContext(Scope *scope) {
    Scope::VarIterator variableIterator(scope);
    while (variableIterator.hasNext()) {
        auto *astVar = variableIterator.next();
        auto *var = new Var(astVar->type(), astVar->name());
        ctx->addVar(var);
    }

    Scope::FunctionIterator functionIterator(scope);
    while (functionIterator.hasNext()) {
        auto *func = functionIterator.next();
        if (containsFunction(func->name())) {
            throw CompileError("Override function", func->node()->position());
        }
        checkFunctionParameters(func);
        ctx->addFun(func);
    }
}

void TypeEvaluter::visitBlockNode(BlockNode *node) {
    ctx = ctx->addChild();
    fillContext(node->scope());
    visitFunctions(node->scope());
    node->visitChildren(this);
    ctx = ctx->getParentContext();
    setType(node, VT_VOID);
}

void TypeEvaluter::visitIfNode(IfNode *node) {
    node->ifExpr()->visit(this);
    VarType ifExprType = getType(node->ifExpr());
    if (ifExprType != VT_INT) {
        throw CompileError("MismatchTypeException", node->position());
    }
    node->thenBlock()->visit(this);
    if (node->elseBlock() != nullptr) {
        node->elseBlock()->visit(this);
    }
    setType(node, VT_VOID);
}

void TypeEvaluter::visitWhileNode(WhileNode *node) {
    node->whileExpr()->visit(this);
    VarType whileType = getType(node->whileExpr());
    if (whileType != VT_INT) {
        throw CompileError("MismatchTypeException", node->position());
    }
    node->loopBlock()->visit(this);
    setType(node, VT_VOID);
}

void TypeEvaluter::visitStoreNode(StoreNode *node) {
    VarType varType = node->var()->type();
    node->value()->visit(this);
    VarType expretionType = getType(node->value());
    if (varType == VT_DOUBLE || varType == VT_INT) {
        if (expretionType != VT_DOUBLE && expretionType != VT_INT) {
            throw CompileError("MismatchTypeException", node->position());
        }
    } else if (varType == VT_STRING) {
        if (expretionType != VT_STRING) {
            throw CompileError("MismatchTypeException", node->position());
        }
    }
    setType(node, VT_VOID);
}

void TypeEvaluter::visitForNode(ForNode *node) {
    node->inExpr()->visit(this);
    VarType iteratorType = node->var()->type();
    if (!node->inExpr()->isBinaryOpNode() || node->inExpr()->asBinaryOpNode()->kind() != tRANGE) {
        throw CompileError("Invalid 'for' operator", node->position());
    }
    if (iteratorType != VT_INT) {
        throw CompileError("MismatchTypeException", node->position());
    }
    node->body()->visit(this);
    setType(node, VT_VOID);
}

void TypeEvaluter::visitReturnNode(ReturnNode *node) {
    if (node->returnExpr() == nullptr) {
        if (returnType != VT_VOID) {
            throw CompileError("MismatchTypeException", node->position());
        }
    } else {
        node->returnExpr()->visit(this);
        VarType type = getType(node->returnExpr());
        if (returnType != VT_DOUBLE || !(type == VT_DOUBLE || type == VT_INT)) {
            if (returnType != type) {
                throw CompileError("MismatchTypeException", node->position());
            }
        }
    }
    setType(node, returnType);
}

void TypeEvaluter::visitPrintNode(PrintNode *node) {
    node->visitChildren(this);
    for (uint32_t i = 0; i < node->operands(); ++i) {
        VarType type = getType(node->operandAt(i));
        if (type == VT_VOID || type == VT_INVALID) {
            throw CompileError("MismatchTypeException", node->position());
        }
    }
    setType(node, VT_VOID);
}

void TypeEvaluter::visitCallNode(CallNode *node) {
    BytecodeFunction *func = ctx->getFunction(node->name());
    if (func == nullptr) {
        throw CompileError("Function not defined", node->position());
    } else if (node->parametersNumber() != func->parametersNumber()) {
        throw CompileError("Wrong number of parameters", node->position());
    }
    for (uint32_t i = 0; i < node->parametersNumber(); ++i) {
        auto *parameter = node->parameterAt(i);
        parameter->visit(this);
        if (checkFunctionCallParameter(func->parameterType(i), getType(parameter)) == VT_INVALID) {
            throw CompileError("MismatchTypeException", parameter->position());
        }
    }
    setType(node, ctx->getFunction(node->name())->returnType());
}

VarType TypeEvaluter::getType(AstNode *node) {
    return *((VarType *) node->info());
}

void TypeEvaluter::setType(AstNode *node, VarType type) {
    node->setInfo(new VarType(type));
}

VarType TypeEvaluter::checkCompareOperation(VarType left, VarType right) {
    if (left != VT_STRING && right != VT_STRING) {
        return VT_INT;
    }
    return VT_INVALID;
}

VarType TypeEvaluter::checkEqualsOperation(VarType left, VarType right) {
    if (left == right || (left != VT_STRING && right != VT_STRING)) {
        return VT_INT;
    }
    return VT_INVALID;
}

VarType TypeEvaluter::checkArithmeticOperation(VarType left, VarType right) {
    if (left == VT_STRING || right == VT_STRING) {
        return VT_INVALID;
    }
    if (left == VT_DOUBLE || right == VT_DOUBLE) {
        return VT_DOUBLE;
    }
    return VT_INT;
}

VarType TypeEvaluter::checkIntegerOperation(VarType left, VarType right) {
    if (left != VT_INT || right != VT_INT) {
        return VT_INVALID;
    }
    return VT_INT;
}

VarType TypeEvaluter::checkRangeOperation(VarType left, VarType right) {
    if (left != VT_INT || right != VT_INT) {
        return VT_INVALID;
    }
    return VT_VOID;
}

bool TypeEvaluter::containsFunction(string name) {
    return ctx->getFunction(name) != nullptr;
}

void TypeEvaluter::visitFunctions(Scope *scope) {
    Scope::FunctionIterator functionIterator(scope);
    VarType currentReturnType = returnType;
    while (functionIterator.hasNext()) {
        auto *func = functionIterator.next();
        returnType = func->returnType();
        func->node()->visit(this);
    }
    returnType = currentReturnType;
}

VarType TypeEvaluter::checkFunctionCallParameter(VarType expected, VarType actual) {
    if (expected == VT_DOUBLE) {
        if (actual != VT_DOUBLE && actual != VT_INT) {
            return VT_INVALID;
        }
    } else if (expected != actual || expected == VT_VOID) {
        return VT_INVALID;
    }
    return expected;
}

void TypeEvaluter::checkFunctionParameters(AstFunction *func) {
    for (uint32_t i = 0; i < func->parametersNumber(); ++i) {
        VarType parameterType = func->parameterType(i);
        if (parameterType == VT_VOID || parameterType == VT_INVALID) {
            throw CompileError("MismatchTypeException", func->node()->position());
        }
    }
}
