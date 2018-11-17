//
// Created by Владислав Калинин on 09/11/2018.
//

#include "bytecode_translator_visitor.h"

using namespace mathvm;

void Bytecode_translator_visitor::visitFunctionNode(mathvm::FunctionNode *node) {
    if (node->name() == "<top>") {
        ctx = new Context();
        node->visit(new TypeEvaluter(ctx, &contextList, &nodeContext, VT_VOID));
    }
    node->body()->visit(this);
}

void Bytecode_translator_visitor::visitBlockNode(BlockNode *node) {
//    ctx = ctx->addChild(node->scope());
//    fillContext(node->scope());
    for (uint32_t i = 0; i < node->nodes(); i++) {
        AstNode *stm = node->nodeAt(i);
        stm->visit(this);
    }
}

void Bytecode_translator_visitor::visitIfNode(IfNode *node) {
//    node->ifExpr()->visit()
}


//==============================[Context]==================================================================

Context *Context::getVarContext(string name) {
    if (parent == nullptr) {
        return nullptr;
    }
    if (variables.find(name) != variables.end()) {
        return this;
    }
    return parent->getVarContext(name);
}

void Context::addVar(Var *var) {
    variables[var->name()] = static_cast<unsigned short>(var_buffer.size());
    var_buffer.push_back(var);
}

void Context::addFun(BytecodeFunction *func) {
    functions[func->name()] = static_cast<unsigned short>(fun_buffer.size());
    fun_buffer.push_back(func);
}

uint16_t Context::count = 0;
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
    if (op == tOR || op == tAND || op == tAOR || op == tAAND || op == tAXOR
        || op == tGT || op == tGE || op == tLT || op == tLE) {
        result = checkBooleanOperation(left, right, op);
    } else if (op == tEQ || op == tNEQ) {
        result = checkEqualsOperation(left, right, op);
    } else if (op == tSUB || op == tMUL || op == tDIV) {
        result = checkArithmeticOperationWithoutPlus(left, right, op);
    } else if (op == tMOD || op == tRANGE) {
        result = checkIntegerModOperation(left, right, op);
    } else if (op == tADD) {
        result = checkPlusOperation(left, right, op);
    }
    if (result == VT_INVALID) {
        throw CompileError("MismatchTypeException", node->position());
    }
    setType(node, result);
}

void TypeEvaluter::visitUnaryOpNode(UnaryOpNode *node) {
    node->visitChildren(this);
    VarType value = getType(node->operand());
    if (value == VT_INT || value == VT_DOUBLE) {
        setType(node, value);
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
    setType(node, VT_VOID);
}

void TypeEvaluter::fillContext(Scope *scope) {
    Scope::FunctionIterator functionIterator(scope);
    VarType currentReturnType = returnType;
    while (functionIterator.hasNext()) {
        auto *funcNode = functionIterator.next();
        returnType = funcNode->returnType();
        funcNode->node()->visit(this);
        auto *func = new BytecodeFunction(funcNode);
        ctx->addFun(func);
    }
    returnType = currentReturnType;

    Scope::VarIterator variableIterator(scope);
    while (variableIterator.hasNext()) {
        auto *astVar = variableIterator.next();
        auto *var = new Var(astVar->type(), astVar->name());
        ctx->addVar(var);
    }
}

void TypeEvaluter::visitBlockNode(BlockNode *node) {
    ctx = ctx->addChild(node->scope());
    registerContext();
    matchNodeAndContext(node);
    fillContext(node->scope());
    AstBaseVisitor::visitBlockNode(node);
    setType(node, VT_VOID);
}

void TypeEvaluter::visitIfNode(IfNode *node) {
    node->ifExpr()->visit(this);
    VarType ifExprType = getType(node->ifExpr());
    if (ifExprType != VT_INT && ifExprType != VT_DOUBLE) {
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
    if (whileType != VT_INT && whileType != VT_DOUBLE) {
        throw CompileError("MismatchTypeException", node->position());
    }
    node->loopBlock()->visit(this);
    setType(node, VT_VOID);
}

void TypeEvaluter::visitStoreNode(StoreNode *node) {
    VarType varType = node->var()->type();
    node->value()->visit(this);
    VarType expretionType = getType(node->value());
    if (varType == VT_DOUBLE) {
        if (expretionType != VT_DOUBLE && expretionType != VT_INT) {
            throw CompileError("MismatchTypeException", node->position());
        }
    } else if (varType == VT_INT) {
        if (expretionType != VT_INT) {
            throw CompileError("MismatchTypeException", node->position());
        }
    } else if (varType == VT_STRING) {
        if (expretionType == VT_INVALID || expretionType == VT_VOID) {
            throw CompileError("MismatchTypeException", node->position());
        }
    }
    setType(node, VT_VOID);
}

void TypeEvaluter::visitForNode(ForNode *node) {
    node->inExpr()->visit(this);
    VarType iteratorType = node->var()->type();
    VarType expressionType = getType(node->inExpr());
    if ((iteratorType != VT_INT && iteratorType != VT_DOUBLE) || expressionType != VT_INT) {
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
        // TODO сделать cast int к double
        if (returnType != type) {
            throw CompileError("MismatchTypeException", node->position());
        }
    }
    setType(node, VT_VOID);
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

void TypeEvaluter::registerContext() {
    (*contextList)[ctx->getId()] = ctx;
}

VarType TypeEvaluter::getType(AstNode *node) {
    return *((VarType *) node->info());
}

void TypeEvaluter::setType(AstNode *node, VarType type) {
    node->setInfo(&type);
}

void TypeEvaluter::matchNodeAndContext(AstNode *node) {
    (*nodeContext)[node->position()] = ctx;
}

VarType TypeEvaluter::checkBooleanOperation(VarType left, VarType right, TokenKind op) {
    if (left != VT_STRING && right != VT_STRING) {
        return VT_INT;
    }
    return VT_INVALID;
}

VarType TypeEvaluter::checkEqualsOperation(VarType left, VarType right, TokenKind op) {
    if (left == right || (left != VT_STRING && right != VT_STRING)) {
        return VT_INT;
    }
    return VT_INVALID;
}

VarType TypeEvaluter::checkArithmeticOperationWithoutPlus(VarType left, VarType right, TokenKind op) {
    if (left == VT_STRING || right == VT_STRING) {
        return VT_INVALID;
    }
    if (left == VT_DOUBLE || right == VT_DOUBLE) {
        return VT_DOUBLE;
    }
    return VT_INT;
}

VarType TypeEvaluter::checkIntegerModOperation(VarType left, VarType right, TokenKind op) {
    if (left != VT_INT || right != VT_INT) {
        return VT_INVALID;
    }
    return VT_INT;
}

VarType TypeEvaluter::checkPlusOperation(VarType left, VarType right, TokenKind op) {
    if (left == VT_STRING || right == VT_STRING) {
        return VT_STRING;
    }
    return checkArithmeticOperationWithoutPlus(left, right, op);
}