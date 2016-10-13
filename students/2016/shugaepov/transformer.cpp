//
// Created by austud on 10/12/16.
//

#include <algorithm>

#include "transformer.h"

namespace mathvm
{

void transformer::visitBinaryOpNode(BinaryOpNode *node) {
    node->left()->visit(this);
    ss << ' ' << transformer::token(node->kind()) << ' ';
    node->right()->visit(this);
}

void transformer::visitBlockNode(BlockNode *node) {
    if (level > 0) ss << transformer::token(TokenKind::tLBRACE);

    level++;

    Scope* scope = node->scope();

    for (Scope::VarIterator varIterator(scope); varIterator.hasNext(); ) {
        AstVar* v = varIterator.next();
        ss << typeToName(v->type()) << ' ' << v->name() << transformer::token(TokenKind::tSEMICOLON);
    }

    for (Scope::FunctionIterator functionIterator(scope); functionIterator.hasNext(); ) {
        functionIterator.next()->node()->visit(this);
    }


    node->visitChildren(this);

    level--;

    if (level > 0) ss << transformer::token(TokenKind::tRBRACE);
}

void transformer::visitDoubleLiteralNode(DoubleLiteralNode *node) {
    ss << node->literal();
}

void transformer::visitForNode(ForNode *node) {
    ss << "for" << ' ' << transformer::token(TokenKind::tLPAREN);
    ss << node->var()->name() << ' ' << "in" << ' ';
    node->inExpr()->visit(this);
    ss << transformer::token(TokenKind::tRPAREN);
    node->body()->visit(this);
}

void transformer::visitFunctionNode(FunctionNode *node) {
    ss << "function" << ' ' << typeToName(node->returnType()) << ' ' << node->name();
    ss << transformer::token(TokenKind::tLPAREN);

    Signature s = node->signature();
    if (s.size() > 1)
        ss << typeToName(s[1].first) << ' ' << s[1].second;
    for (size_t i = 2; i < s.size(); i++)
        ss << transformer::token(TokenKind::tCOMMA) << ' ' << typeToName(s[i].first) << ' ' << s[i].second;

    ss << transformer::token(TokenKind::tRPAREN);

    if (node->body()->nodes() > 0 && node->body()->nodeAt(0)->isNativeCallNode()) {
        node->body()->nodeAt(0)->visit(this);
    }
    else {
        node->body()->visit(this);
    }
}

void transformer::visitIfNode(IfNode *node) {
    ss << "if" << transformer::token(TokenKind::tLPAREN);
    node->ifExpr()->visit(this);
    ss << transformer::token(TokenKind::tRPAREN);

    node->thenBlock()->visit(this);
    if (node->elseBlock()) {
        ss << "else";
        node->elseBlock()->visit(this);
    }
}

void transformer::visitIntLiteralNode(IntLiteralNode *node) {
    ss << node->literal();
}

void transformer::visitLoadNode(LoadNode *node) {
    ss << node->var()->name();
}

void transformer::visitNativeCallNode(NativeCallNode *node) {
    ss << "native" << ' ' << "'" << node->nativeName() << "'" << transformer::token(TokenKind::tSEMICOLON);
}

void transformer::visitPrintNode(PrintNode *node) {
    ss << "print" << ' ' << transformer::token(TokenKind::tLPAREN);
    if (node->operands() > 0)
        node->operandAt(0)->visit(this);
    for (size_t i = 1; i < node->operands(); i++) {
        ss << transformer::token(TokenKind::tCOMMA) << ' ';
        node->operandAt(i)->visit(this);
    }
    ss << transformer::token(TokenKind::tRPAREN);
    ss << transformer::token(TokenKind::tSEMICOLON);
}

void transformer::visitReturnNode(ReturnNode *node) {
    ss << "return" << ' ';
    if (node->returnExpr())
        node->returnExpr()->visit(this);
    ss << transformer::token(TokenKind::tSEMICOLON);
}

void transformer::visitStoreNode(StoreNode *node) {
    ss << node->var()->name() << ' ' << transformer::token(node->op()) << ' ';
    node->value()->visit(this);
    ss << transformer::token(TokenKind::tSEMICOLON);
}

string transformer::string_literal_transform(string l) {
    string res;

    for (char c : l) {
        switch (c) {
            case '\n':
                res.push_back('\\'), res.push_back('n');
                break;
            case '\r':
                res.push_back('\\'), res.push_back('r');
                break;
            case '\t':
                res.push_back('\\'), res.push_back('t');
                break;
            case '\\':
                res.push_back('\\'), res.push_back('\\');
                break;
            default:
                res.push_back(c);
        }
    }

    return res;
}

void transformer::visitStringLiteralNode(StringLiteralNode *node) {
    string l = node->literal();

    ss << "'" << transformer::string_literal_transform(l) << "'";
}

void transformer::visitUnaryOpNode(UnaryOpNode *node) {
    ss << transformer::token(node->kind());
    node->operand()->visit(this);
}

void transformer::visitWhileNode(WhileNode *node) {
    ss << "while" << transformer::token(TokenKind::tLPAREN);
    node->whileExpr()->visit(this);
    ss << transformer::token(TokenKind::tRPAREN);
    node->loopBlock()->visit(this);
}

void transformer::visitCallNode(CallNode *node) {
    ss << node->name() << transformer::token(TokenKind::tLPAREN);

    if (node->parametersNumber() > 0)
        node->parameterAt(0)->visit(this);

    for (size_t i = 1; i < node->parametersNumber(); i++) {
        ss << transformer::token(TokenKind::tCOMMA) << ' ';
        node->parameterAt(i)->visit(this);
    }
    ss << transformer::token(TokenKind::tRPAREN);
}

} // mathvm
