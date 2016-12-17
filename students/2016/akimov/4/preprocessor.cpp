#include "preprocessor.h"
#include <dlfcn.h>

using namespace mathvm;


static void error(AstNode* node, const string& message = "") {
    (new TypeError())->error(node->position(), "TypeError: %s", message.c_str());
}

VarType Preprocessor::getType(AstNode* node) {
    if (!_types.count(node)) error(node, "type is not set");
    return _types[node];
}

void Preprocessor::setType(AstNode* node, VarType type) {
    if (_types.count(node)) error(node, "type already set");
    _types[node] = type;
}


static VarType combineTypes(VarType arg1, VarType arg2) {
    return (arg1 == VT_DOUBLE || arg2 == VT_DOUBLE) ? VT_DOUBLE : VT_INT;
}

void checkInt(VarType type, AstNode* node) {
    if (type != VT_INT) error(node);
}

void checkIntOrDouble(VarType type, AstNode* node) {
    if (type != VT_INT && type != VT_DOUBLE) error(node);
}

void checkValueType(VarType type, AstNode* node) {
    if (type != VT_INT && type != VT_DOUBLE && type != VT_STRING) error(node);
}


void Preprocessor::visitBinaryOpNode(BinaryOpNode* node) {
    node->left()->visit(this);
    node->right()->visit(this);

    VarType leftType = getType(node->left());
    VarType rightType = getType(node->right());
    VarType resultType = VT_INVALID;

    checkIntOrDouble(leftType, node);
    checkIntOrDouble(rightType, node);

    switch (node->kind()) {
        case tOR:
        case tAND:
            checkInt(leftType, node);
            checkInt(rightType, node);
            resultType = VT_INT;
            break;

        case tAOR:
        case tAAND:
        case tAXOR:
            checkInt(leftType, node);
            checkInt(rightType, node);
            resultType = VT_INT;
            break;

        case tEQ:
        case tNEQ:
        case tGT:
        case tGE:
        case tLT:
        case tLE:
            resultType = VT_INT;
            break;

        case tRANGE:
            checkInt(leftType, node);
            checkInt(rightType, node);
            resultType = VT_VOID;
            break;

        case tADD:
        case tSUB:
        case tMUL:
        case tDIV:
            resultType = combineTypes(leftType, rightType);
            break;

        case tMOD:
            checkInt(leftType, node);
            checkInt(rightType, node);
            resultType = VT_INT;
            break;

        default:
            error(node);
    }

    setType(node, resultType);
}

void Preprocessor::visitUnaryOpNode(UnaryOpNode* node) {
    node->operand()->visit(this);

    VarType type = getType(node->operand());
    TokenKind kind = node->kind();

    switch (kind) {
        case tSUB:
            checkIntOrDouble(type, node);
            setType(node, type);
            break;

        case tNOT:
            setType(node, VT_INT);
            break;

        default:
            error(node);
    }
}

void Preprocessor::visitStringLiteralNode(StringLiteralNode* node) {
    setType(node, VT_STRING);
}

void Preprocessor::visitDoubleLiteralNode(DoubleLiteralNode* node) {
    setType(node, VT_DOUBLE);
}

void Preprocessor::visitIntLiteralNode(IntLiteralNode* node) {
    setType(node, VT_INT);
}

void Preprocessor::visitLoadNode(LoadNode* node) {
    setType(node, node->var()->type());
}

void Preprocessor::visitStoreNode(StoreNode* node) {
    node->value()->visit(this);

    VarType valueType = getType(node->value());
    checkValueType(valueType, node);

    setType(node, VT_VOID);
}

void Preprocessor::visitForNode(ForNode* node) {
    node->inExpr()->visit(this);
    node->body()->visit(this);

    setType(node, VT_VOID);
}

void Preprocessor::visitWhileNode(WhileNode* node) {
    node->whileExpr()->visit(this);
    node->loopBlock()->visit(this);

    checkValueType(getType(node->whileExpr()), node);

    setType(node, VT_VOID);
}

void Preprocessor::visitIfNode(IfNode* node) {
    node->ifExpr()->visit(this);
    node->thenBlock()->visit(this);
    if (node->elseBlock()) {
        node->elseBlock()->visit(this);
    }

    checkValueType(getType(node->ifExpr()), node);

    setType(node, VT_VOID);
}

void Preprocessor::visitBlockNode(BlockNode* node) {
    _scopes.push(node->scope());
    for (Scope::FunctionIterator it(node->scope()); it.hasNext();) {
        visitFunctionNode(it.next()->node());
    }
    for (uint32_t i = 0; i < node->nodes(); ++i) {
        node->nodeAt(i)->visit(this);
    }
    _scopes.pop();

    setType(node, VT_VOID);
}

void Preprocessor::visitFunctionNode(FunctionNode* node) {
    setType(node, node->returnType());

    _functions.push(node);
    node->body()->visit(this);
    _functions.pop();
}

void Preprocessor::visitReturnNode(ReturnNode* node) {
    if (node->returnExpr()) {
        node->returnExpr()->visit(this);

        VarType returnType = getType(_functions.top());
        VarType exprType = getType(node->returnExpr());
        checkValueType(exprType, node);

        setType(node, returnType);
    } else {
        setType(node, VT_VOID);
    }
}

void Preprocessor::visitCallNode(CallNode* node) {
    FunctionNode* function = _scopes.top()->lookupFunction(node->name())->node();

    if (node->parametersNumber() != function->parametersNumber())
        error(node);

    for (uint32_t i = 0; i < node->parametersNumber(); ++i) {
        node->parameterAt(i)->visit(this);

        VarType type = getType(node->parameterAt(i));
        checkValueType(type, node);
    }

    setType(node, function->returnType());
}

void Preprocessor::visitNativeCallNode(NativeCallNode* node) {
    void* ptr = dlsym(RTLD_DEFAULT, node->nativeName().c_str());
    if (!ptr) error(node, dlerror());
    node->setInfo(ptr);

    setType(node, VT_VOID);
}

void Preprocessor::visitPrintNode(PrintNode* node) {
    for (uint32_t i = 0; i < node->operands(); ++i) {
        node->operandAt(i)->visit(this);

        VarType type = getType(node->operandAt(i));
        checkValueType(type, node);
    }

    setType(node, VT_VOID);
}
