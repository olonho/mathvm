//
// Created by svloyso on 17.10.16.
//

#include "ast_to_code.h"
#include "mathvm.h"

namespace mathvm {

void AstToCode::dumpVar(const AstVar* var) {
    result << typeToName(var->type()) << space() << var->name();
}

void AstToCode::dumpBlock(BlockNode *block, bool mandatoryBraces) {
    if (mandatoryBraces || block->scope()->variablesCount() != 0 || block->scope()->functionsCount() != 0 || block->nodes() > 1)
        result << lbr() << line();
    incrIndent();
    block->visit(this);
    decrIndent();
    if (mandatoryBraces || block->scope()->variablesCount() != 0 || block->scope()->functionsCount() != 0 || block->nodes() > 1)
        result << indent() << rbr();
}

void AstToCode::dumpCode(AstNode *root) {
    root->visit(this);
}

void AstToCode::visitUnaryOpNode(UnaryOpNode *node) {
    result << tokenOp(node->kind());
    if(node->operand()->isBinaryOpNode() || node->operand()->isUnaryOpNode()) result << lpar();
    node->visitChildren(this);
    if(node->operand()->isBinaryOpNode() || node->operand()->isUnaryOpNode()) result << lpar();
}

void AstToCode::visitBinaryOpNode(BinaryOpNode *node) {
    int prec = tokenPrecedence(node->kind());
    int leftPrec = node->left()->isUnaryOpNode()
                   ? tokenPrecedence(node->left()->asUnaryOpNode()->kind())
                   : node->left()->isBinaryOpNode()
                     ? tokenPrecedence(node->left()->asBinaryOpNode()->kind())
                     : 99;
    int rightPrec = node->right()->isUnaryOpNode()
                    ? tokenPrecedence(node->right()->asUnaryOpNode()->kind())
                    : node->right()->isBinaryOpNode()
                      ? tokenPrecedence(node->right()->asBinaryOpNode()->kind())
                      : 99;

    if(leftPrec < prec) result << lpar();
    node->left()->visit(this);
    if(leftPrec < prec) result << rpar();
    result << space() << tokenOp(node->kind()) << space();
    if(rightPrec < prec) result << lpar();
    node->right()->visit(this);
    if(rightPrec < prec) result << rpar();
}

static std::string escapeCharacter(char ch) {
    switch (ch) {
        case '\'':
            return "\\'";
        case '\"':
            return "\\'";
        case '\?':
            return "\\?";
        case '\\':
            return "\\";
        case '\a':
            return "\\a";
        case '\b':
            return "\\b";
        case '\f':
            return "\\f";
        case '\n':
            return "\\n";
        case '\r':
            return "\\r";
        case '\t':
            return "\\t";
        case '\v':
            return "\\v";
        default:
            return std::string(1, ch);
    }
}

void AstToCode::visitStringLiteralNode(StringLiteralNode *node) {
    result << quot();
    for(char ch : node->literal()) {
        result << escapeCharacter(ch);
    }
    result << quot();
}

void AstToCode::visitIntLiteralNode(IntLiteralNode *node) {
    result << node->literal();
}

void AstToCode::visitDoubleLiteralNode(DoubleLiteralNode *node) {
    result << node->literal();
}

void AstToCode::visitLoadNode(LoadNode *node) {
    result << node->var()->name();
}

void AstToCode::visitStoreNode(StoreNode *node) {
    result << node->var()->name() << space() << tokenOp(node->op()) << space();
    node->value()->visit(this);
}

void AstToCode::visitBlockNode(BlockNode *node) {
    for(Scope::VarIterator it(node->scope()); it.hasNext();) {
        AstVar* var = it.next();
        result << indent();
        dumpVar(var);
        result << semi() << line();
    }

    for(Scope::FunctionIterator it(node->scope()); it.hasNext();) {
        AstFunction* func = it.next();
        result << indent();
        func->node()->visit(this);
        result << line() << line();
    }

    for(int i = 0; i < node->nodes(); ++i) {
        AstNode* child = node->nodeAt(i);
        result << indent();
        child->visit(this);
        if(!child->isIfNode() && !child->isForNode() && !child->isWhileNode() && !child->isBlockNode() && !child->isFunctionNode())
            result << semi() << line();
    }
}

void AstToCode::visitNativeCallNode(NativeCallNode *node) {
    result << "native" << space() << quot() << node->nativeName() << quot();
}

void AstToCode::visitForNode(ForNode *node) {
    result << "for" << lpar();
    dumpVar(node->var());
    result << space() << "in" << space();
    node->inExpr()->visit(this);
    result << rpar() << space();
    dumpBlock(node->body());
    result << line();
}

void AstToCode::visitWhileNode(WhileNode *node) {
    result << "while" << lpar();
    node->whileExpr()->visit(this);
    result << rpar() << space();
    dumpBlock(node->loopBlock());
    result << line();
}

void AstToCode::visitIfNode(IfNode *node) {
    result << "if" << lpar();
    node->ifExpr()->visit(this);
    result << rpar() << space();
    dumpBlock(node->thenBlock());
    if(node->elseBlock()) {
        result << space() << "else" << space();
        dumpBlock(node->elseBlock());
    }
    result << line();
}

void AstToCode::visitReturnNode(ReturnNode *node) {
    result << "return" << space();
    node->returnExpr()->visit(this);
}

void AstToCode::visitFunctionNode(FunctionNode *node) {
    result << "function" << space() << typeToName(node->returnType()) << space() << node->name() << lpar();
    for(int i = 0; i < node->parametersNumber(); ++i) {
        if(i) result << com() << space();
        result << typeToName(node->parameterType(i)) << space() << node->parameterName(i);
    }
    result << rpar() << space();
    dumpBlock(node->body(), true);
}

void AstToCode::visitCallNode(CallNode *node) {
    result << node->name() << lpar();
    for(int i = 0; i < node->parametersNumber(); ++i) {
        if(i) result << com() << space();
        AstNode* par = node->parameterAt(i);
        par->visit(this);
    }
    result << rpar();
}

void AstToCode::visitPrintNode(PrintNode *node) {
    result << "print" << lpar();
    for(int i = 0; i < node->operands(); ++i) {
        if(i) result << com() << space();
        AstNode* op = node->operandAt(i);
        op->visit(this);
    }
    result << rpar();
}

}