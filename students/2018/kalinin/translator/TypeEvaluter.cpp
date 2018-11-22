//
// Created by Владислав Калинин on 20/11/2018.
//

#include "TypeEvaluter.h"

using namespace mathvm;

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
    for (uint32_t i = 0; i < node->parametersNumber(); ++i) {
        ctx->getLastChildren()->addVar(new Var(node->parameterType(i), node->parameterName(i)));
    }
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
