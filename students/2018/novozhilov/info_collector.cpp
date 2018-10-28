#include <ast.h>
#include "include/bytecode_generator.h"

using namespace mathvm;

BytecodeGenerator::TypeInfoCollector::TypeInfoCollector(BytecodeGenerator *bytecodeGenerator)
        : _bytecodeGenerator(bytecodeGenerator), _returnTypes(), _info(_bytecodeGenerator->_info) {}

void BytecodeGenerator::TypeInfoCollector::visitForNode(ForNode *node) {
    VarType type = node->var()->type();
    uint32_t index = node->inExpr()->position();
    _info.expressionType[index] = type;
    node->inExpr()->visit(this);

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
    _info.expressionType[node->ifExpr()->position()] = VT_INT;
    node->visitChildren(this);
}

void BytecodeGenerator::TypeInfoCollector::visitWhileNode(WhileNode *node) {
    _info.expressionType[node->whileExpr()->position()] = VT_INT;
    node->visitChildren(this);
}

void BytecodeGenerator::TypeInfoCollector::visitBlockNode(BlockNode *node) {
    node->visitChildren(this);
}

void BytecodeGenerator::TypeInfoCollector::visitBinaryOpNode(BinaryOpNode *node) {
    VarType type = getNodeType(node);

    if (type != VT_INVALID) {
        _info.expressionType[node->left()->position()] = type;
        _info.expressionType[node->right()->position()] = type;
        node->visitChildren(this);
    } else {
        node->left()->visit(this);
        type = getNodeType(node->left());
        assert(type != VT_INVALID);
        _info.expressionType[node->position()] = type;
        _info.expressionType[node->right()->position()] = type;
        node->right()->visit(this);
    }
}

void BytecodeGenerator::TypeInfoCollector::visitUnaryOpNode(UnaryOpNode *node) {
    VarType type = getNodeType(node);
    if (type != VT_INVALID) {
        _info.expressionType[node->operand()->position()] = type;
        node->visitChildren(this);
    } else {
        node->visitChildren(this);
        type = getNodeType(node->operand());
        assert(type != VT_INVALID);
        _info.expressionType[node->position()] = type;
    }
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
    if (node->returnExpr() != nullptr) {
        _info.expressionType[node->returnExpr()->position()] = _returnTypes.top();
    }
    node->visitChildren(this);
}

void BytecodeGenerator::TypeInfoCollector::visitStoreNode(StoreNode *node) {
    VarType type = node->var()->type();
    _info.expressionType[node->value()->position()] = type;
    node->visitChildren(this);
}

void BytecodeGenerator::TypeInfoCollector::visitCallNode(CallNode *node) {
    uint32_t index = node->position();
    TranslatedFunction *function = _bytecodeGenerator->_code->functionByName(node->name());

    VarType type = getNodeType(node);
    _info.expressionType[index] = type != VT_INVALID ? _info.expressionType[index] : function->returnType();


    uint16_t parametersNumber = function->parametersNumber();

    for (uint16_t i = 0; i < parametersNumber; ++i) {
        uint32_t parameterIndex = node->parameterAt(i)->position();
        _info.expressionType[parameterIndex] = function->parameterType(i);
    }
    node->visitChildren(this);
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


