#include <ast.h>
#include "include/bytecode_generator.h"

using namespace mathvm;

BytecodeGenerator::TypeInfoCollector::TypeInfoCollector(BytecodeGenerator *bytecodeGenerator)
        : _bytecodeGenerator(bytecodeGenerator), _returnTypes(), _info(_bytecodeGenerator->_info) {}

void BytecodeGenerator::TypeInfoCollector::visitForNode(ForNode *node) {
    node->inExpr()->visit(this);

    VarType type = node->var()->type();
    uint32_t index = node->inExpr()->position();
    _info.expressionType[index] = type;

    node->body()->visit(this);
}

void BytecodeGenerator::TypeInfoCollector::visitPrintNode(PrintNode *node) {
    node->visitChildren(this);
}

void BytecodeGenerator::TypeInfoCollector::visitLoadNode(LoadNode *node) {
    VarType type = node->var()->type();
    _info.expressionType[node->position()] = type;
}

void BytecodeGenerator::TypeInfoCollector::visitIfNode(IfNode *node) {
    node->visitChildren(this);
    _info.expressionType[node->ifExpr()->position()] = VT_INT;
}

void BytecodeGenerator::TypeInfoCollector::visitWhileNode(WhileNode *node) {
    node->visitChildren(this);
    _info.expressionType[node->whileExpr()->position()] = VT_INT;
}

void BytecodeGenerator::TypeInfoCollector::visitBlockNode(BlockNode *node) {
    node->visitChildren(this);
}

void BytecodeGenerator::TypeInfoCollector::visitBinaryOpNode(BinaryOpNode *node) {
    node->visitChildren(this);
    VarType type;
    if (isArithmeticOperation(node->kind())) {
        VarType leftType = getNodeType(node->left());
        VarType rightType = getNodeType(node->right());
        if (leftType == VT_DOUBLE || rightType == VT_DOUBLE) {
            type = VT_DOUBLE;
        } else {
            type = VT_INT;
        }
    } else {
        type = VT_INT;
    }
    _info.expressionType[node->left()->position()] = type;
    _info.expressionType[node->right()->position()] = type;
    _info.expressionType[node->position()] = type;
}

void BytecodeGenerator::TypeInfoCollector::visitUnaryOpNode(UnaryOpNode *node) {
    node->visitChildren(this);
    VarType type = getNodeType(node->operand());
    assert(type != VT_INVALID);
    _info.expressionType[node->position()] = type;

}

void BytecodeGenerator::TypeInfoCollector::visitNativeCallNode(NativeCallNode *node) {
    // TODO
    AstVisitor::visitNativeCallNode(node);
}

void BytecodeGenerator::TypeInfoCollector::visitFunctionNode(FunctionNode *node) {
    _returnTypes.push(node->returnType());
    node->visitChildren(this);
    _returnTypes.pop();
}

void BytecodeGenerator::TypeInfoCollector::visitReturnNode(ReturnNode *node) {
    node->visitChildren(this);
    if (node->returnExpr() != nullptr) {
        _info.expressionType[node->returnExpr()->position()] = _returnTypes.top();
    }
}

void BytecodeGenerator::TypeInfoCollector::visitStoreNode(StoreNode *node) {
    VarType type = node->var()->type();
    node->visitChildren(this);
    _info.expressionType[node->value()->position()] = type;
}

void BytecodeGenerator::TypeInfoCollector::visitCallNode(CallNode *node) {
    uint32_t index = node->position();
    TranslatedFunction *function = _bytecodeGenerator->_code->functionByName(node->name());

    _info.expressionType[index] = function->returnType();

    uint16_t parametersNumber = function->parametersNumber();

    node->visitChildren(this);
    for (uint16_t i = 0; i < parametersNumber; ++i) {
        uint32_t parameterIndex = node->parameterAt(i)->position();
        _info.expressionType[parameterIndex] = function->parameterType(i);
    }
}

void BytecodeGenerator::TypeInfoCollector::visitIntLiteralNode(IntLiteralNode *node) {
    if (getNodeType(node) == VT_INVALID) {
        _info.expressionType[node->position()] = VT_INT;
    }
}

void BytecodeGenerator::TypeInfoCollector::visitDoubleLiteralNode(DoubleLiteralNode *node) {
    if (getNodeType(node) == VT_INVALID) {
        _info.expressionType[node->position()] = VT_DOUBLE;
    }
}

void BytecodeGenerator::TypeInfoCollector::visitStringLiteralNode(StringLiteralNode *node) {
    if (getNodeType(node) == VT_INVALID) {
        _info.expressionType[node->position()] = VT_STRING;
    }
}

VarType BytecodeGenerator::TypeInfoCollector::getNodeType(AstNode *node) {
    return _info.expressionType.find(node->position()) == _info.expressionType.end()
           ? VT_INVALID
           : _info.expressionType[node->position()];
}

bool BytecodeGenerator::TypeInfoCollector::isArithmeticOperation(TokenKind const &kind) {
    switch (kind) {
        case tADD:
        case tSUB:
        case tMUL:
        case tDIV:
        case tMOD:
            return true;
        default:
            return false;
    }
}

// -------------------------------------------------------------------------------------------

BytecodeGenerator::FunctionCollector::FunctionCollector(BytecodeGenerator *_bytecodeGenerator)
        : _bytecodeGenerator(_bytecodeGenerator) {}

void BytecodeGenerator::FunctionCollector::visitFunctionNode(FunctionNode *node) {
    node->visitChildren(this);
}

void BytecodeGenerator::FunctionCollector::visitBlockNode(BlockNode *node) {
    Scope::FunctionIterator functionIterator(node->scope());
    while (functionIterator.hasNext()) {
        AstFunction *function = functionIterator.next();
        auto *bytecodeFunction = new BytecodeFunction(function);
        uint16_t functionId = _bytecodeGenerator->_code->addFunction(bytecodeFunction);
        bytecodeFunction->setScopeId(functionId);
        function->setInfo(bytecodeFunction);
        function->node()->visit(this);
    }
}

// -------------------------------------------------------------------------------------------

BytecodeGenerator::FunctionCallCollector::FunctionCallCollector(BytecodeGenerator *_bytecodeGenerator)
    : _bytecodeGenerator(_bytecodeGenerator), _info(_bytecodeGenerator->_info), _parentIsBlockNode(false) {}

void BytecodeGenerator::FunctionCallCollector::visitFunctionNode(FunctionNode *node) {
    node->visitChildren(this);
}

void BytecodeGenerator::FunctionCallCollector::visitBlockNode(BlockNode *node) {
    Scope::FunctionIterator functionIterator(node->scope());
    while (functionIterator.hasNext()) {
        functionIterator.next()->node()->visit(this);
    }

    uint32_t nodesNumber = node->nodes();
    for (uint32_t i = 0; i < nodesNumber; ++i) {
        _parentIsBlockNode = true;
        node->nodeAt(i)->visit(this);
    }
}

void BytecodeGenerator::FunctionCallCollector::visitForNode(ForNode *node) {
    _parentIsBlockNode = false;
    node->visitChildren(this);
}

void BytecodeGenerator::FunctionCallCollector::visitPrintNode(PrintNode *node) {
    _parentIsBlockNode = false;
    node->visitChildren(this);
}

void BytecodeGenerator::FunctionCallCollector::visitIfNode(IfNode *node) {
    _parentIsBlockNode = false;
    node->visitChildren(this);
}

void BytecodeGenerator::FunctionCallCollector::visitCallNode(CallNode *node) {
    _info.returnValueUsed[node->position()] = !_parentIsBlockNode;
    bool oldParentIsBlockNode = _parentIsBlockNode;
    _parentIsBlockNode = false;
    node->visitChildren(this);
    _parentIsBlockNode = oldParentIsBlockNode;
}

void BytecodeGenerator::FunctionCallCollector::visitStoreNode(StoreNode *node) {
    _parentIsBlockNode = false;
    node->visitChildren(this);
}

void BytecodeGenerator::FunctionCallCollector::visitWhileNode(WhileNode *node) {
    _parentIsBlockNode = false;
    node->visitChildren(this);
}

void BytecodeGenerator::FunctionCallCollector::visitBinaryOpNode(BinaryOpNode *node) {
    _parentIsBlockNode = false;
    node->visitChildren(this);
}

void BytecodeGenerator::FunctionCallCollector::visitUnaryOpNode(UnaryOpNode *node) {
    _parentIsBlockNode = false;
    node->visitChildren(this);
}

void BytecodeGenerator::FunctionCallCollector::visitNativeCallNode(NativeCallNode *node) {
    // TODO
    AstVisitor::visitNativeCallNode(node);
}

void BytecodeGenerator::FunctionCallCollector::visitReturnNode(ReturnNode *node) {
    _parentIsBlockNode = false;
    node->visitChildren(this);
}


