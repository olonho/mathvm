//
// Created by andy on 11/13/16.
//

#include "type_deducer.h"
namespace mathvm
{
void TypeDeducer::visitWhileNode(WhileNode *node) {
    _nodeType.insert({node, VT_INVALID});
}

void TypeDeducer::visitStringLiteralNode(StringLiteralNode *node) {
    _nodeType.insert({node, VT_STRING});
}

void TypeDeducer::visitBinaryOpNode(BinaryOpNode *node) {
    node->left()->visit(this);
    node->right()->visit(this);

    switch (node->kind()) {
        case tASSIGN:
            _nodeType.insert({node, getNodeType(node->left())});
            break;
        case tOR:
        case tAND:
        case tAOR:
        case tAAND:
        case tAXOR:
        case tRANGE:
        case tEQ:
        case tNEQ:
        case tGT:
        case tGE:
        case tLT:
        case tLE:
        case tMOD:
            _nodeType.insert({node, VT_INT});
            break;
        case tADD:
        case tSUB:
        case tMUL:
        case tDIV:
            if (getNodeType(node->left()) == VT_DOUBLE || getNodeType(node->right()) == VT_DOUBLE) {
                _nodeType.insert({node, VT_DOUBLE});
            } else {
                _nodeType.insert({node, VT_INT});
            }
            break;

        default:
            _nodeType.insert({node, VT_INVALID});
    }
}

void TypeDeducer::visitBlockNode(BlockNode *node) {
    _nodeType.insert({node, VT_INVALID});
}

void TypeDeducer::visitCallNode(CallNode *node) {
    _nodeType.insert({node, code->functionByName(node->name())->returnType()});
}

void TypeDeducer::visitDoubleLiteralNode(DoubleLiteralNode *node) {
    _nodeType.insert({node, VT_DOUBLE});
}

void TypeDeducer::visitForNode(ForNode *node) {
    _nodeType.insert({node, VT_INVALID});
}

void TypeDeducer::visitReturnNode(ReturnNode *node) {
    _nodeType.insert({node, VT_INVALID});
}

void TypeDeducer::visitUnaryOpNode(UnaryOpNode *node) {
    if (node->kind() == tNOT) {
        _nodeType.insert({node, VT_INT});
    } else {
        node->operand()->visit(this);
        _nodeType.insert({node, getNodeType(node->operand())});
    }
}

void TypeDeducer::visitFunctionNode(FunctionNode *node) {
    _nodeType.insert({node, VT_INVALID});
}

void TypeDeducer::visitIfNode(IfNode *node) {
    _nodeType.insert({node, VT_INVALID});
}

void TypeDeducer::visitLoadNode(LoadNode *node) {
    _nodeType.insert({node, node->var()->type()});
}

void TypeDeducer::visitStoreNode(StoreNode *node) {
    _nodeType.insert({node, VT_INVALID}); // TODO
}

void TypeDeducer::visitNativeCallNode(NativeCallNode *node) {
    _nodeType.insert({node, node->nativeSignature()[0].first});
}

void TypeDeducer::visitIntLiteralNode(IntLiteralNode *node) {
    _nodeType.insert({node, VT_INT});
}

void TypeDeducer::visitPrintNode(PrintNode *node) {
    _nodeType.insert({node, VT_VOID});
}

VarType TypeDeducer::getNodeType(AstNode *node) {
    auto res = _nodeType.find(node);
    if (res != _nodeType.end()) {
        return res->second;
    }

    if (!node) {
        return VT_INVALID;
    }

    node->visit(this);
    res = _nodeType.find(node);
    if (res == _nodeType.end()) {
        return VT_INVALID;
    }
    return res->second;
}


}