#include <ast.h>
#include "include/bytecode_generator.h"

using namespace mathvm;

void BytecodeGenerator::InfoCollector::visitForNode(ForNode *node) {
    VarType type = node->var()->type();
    uint32_t index = node->inExpr()->position();
    _bytecodeGenerator->_info.expressionType[index] = type;
    node->inExpr()->visit(this);

    node->body()->visit(this);
}

void BytecodeGenerator::InfoCollector::visitPrintNode(PrintNode *node) {
    node->visitChildren(this);
}

void BytecodeGenerator::InfoCollector::visitLoadNode(LoadNode *node) {
    VarType type = node->var()->type();
    _bytecodeGenerator->_info.expressionType[node->position()] = type;
}

void BytecodeGenerator::InfoCollector::visitIfNode(IfNode *node) {
    _bytecodeGenerator->_info.expressionType[node->ifExpr()->position()] = VT_INT;
    node->visitChildren(this);
}

void BytecodeGenerator::InfoCollector::visitWhileNode(WhileNode *node) {
    _bytecodeGenerator->_info.expressionType[node->whileExpr()->position()] = VT_INT;
    node->visitChildren(this);
}

void BytecodeGenerator::InfoCollector::visitBlockNode(BlockNode *node) {
    node->visitChildren(this);
}

void BytecodeGenerator::InfoCollector::visitBinaryOpNode(BinaryOpNode *node) {
    VarType type = _bytecodeGenerator->_info.expressionType[node->position()];
    _bytecodeGenerator->_info.expressionType[node->left()->position()] = type;
    _bytecodeGenerator->_info.expressionType[node->right()->position()] = type;
    node->visitChildren(this);
}

void BytecodeGenerator::InfoCollector::visitUnaryOpNode(UnaryOpNode *node) {
    _bytecodeGenerator->_info.expressionType[node->operand()->position()] = _bytecodeGenerator->_info.expressionType[node->position()];
    node->visitChildren(this);
}

void BytecodeGenerator::InfoCollector::visitNativeCallNode(NativeCallNode *node) {
    // TODO
    AstVisitor::visitNativeCallNode(node);
}

void BytecodeGenerator::InfoCollector::visitFunctionNode(FunctionNode *node) {
    _returnTypes.push(node->returnType());
    node->visitChildren(this);
    _returnTypes.pop();
}

void BytecodeGenerator::InfoCollector::visitReturnNode(ReturnNode *node) {
    if (node->returnExpr() != nullptr) {
        _bytecodeGenerator->_info.expressionType[node->returnExpr()->position()] = _returnTypes.top();
    }
    node->visitChildren(this);
}

void BytecodeGenerator::InfoCollector::visitStoreNode(StoreNode *node) {
    VarType type = node->var()->type();
    _bytecodeGenerator->_info.expressionType[node->value()->position()] = type;
    node->visitChildren(this);
}

void BytecodeGenerator::InfoCollector::visitCallNode(CallNode *node) {
    uint32_t index = node->position();
    bool callUsedInExpression = _bytecodeGenerator->_info.expressionType.find(index) != _bytecodeGenerator->_info.expressionType.end();
    _bytecodeGenerator->_info.returnValueUsed[index] = callUsedInExpression;
    _bytecodeGenerator->_info.expressionType[index] = callUsedInExpression ? _bytecodeGenerator->_info.expressionType[index] : VT_VOID;

    TranslatedFunction *function = _bytecodeGenerator->_code->functionByName(node->name());
    uint16_t parametersNumber = function->parametersNumber();

    for (uint16_t i = 0; i < parametersNumber; ++i) {
        uint32_t parameterIndex = node->parameterAt(i)->position();
        _bytecodeGenerator->_info.expressionType[parameterIndex] = function->parameterType(i);
    }
    node->visitChildren(this);
}

BytecodeGenerator::InfoCollector::InfoCollector(BytecodeGenerator *bytecodeGenerator) 
    : _bytecodeGenerator(bytecodeGenerator), _returnTypes() {}
