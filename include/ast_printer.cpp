/*
 * ast_printer.cpp
 *
 *  Created on: Sep 23, 2012
 *      Author: Alexander Opeykin (alexander.opeykin@gmail.com)
 */
#include "ast_printer.h"

namespace mathvm {

string typeToString(VarType type) {
    switch (type) {
        case VT_INVALID: return "INVALID";
        case VT_VOID:    return "void";
        case VT_DOUBLE:  return "double";
        case VT_INT:     return "int";
        case VT_STRING:  return "string";
        default:         return "UNKNOWN_TYPE";
    }
}

string kindToString(TokenKind kind) {

#define OPERATION_PRINT(code, str, _) \
    if (code == kind) return str; \

    FOR_TOKENS(OPERATION_PRINT)
#undef OPERATION_PRINT

    return "UNKNOWN_OPERATION";
}

string escape(char c) {
    switch (c) {
        case '\n': return "\\n";
        case '\t': return "\\t";
        case '\b': return "\\b";
        case '\r': return "\\r";
        case '\\': return "\\\\";
        default:   return string(1, c);
    }
}

AstPrinter::~AstPrinter() {
}

void AstPrinter::visitBinaryOpNode(BinaryOpNode* node) {
    _ostrm << '(';
    node->left()->visit(this);
    _ostrm << kindToString(node->kind());
    node->right()->visit(this);
    _ostrm << ')';
}

void AstPrinter::visitUnaryOpNode(UnaryOpNode* node) {
    _ostrm << kindToString(node->kind());
    node->operand()->visit(this);
}

void AstPrinter::visitStringLiteralNode(StringLiteralNode* node) {
    _ostrm << '\'';
    const string& str = node->literal();
    for (uint32_t i = 0; i < str.size(); ++i) {
        _ostrm << escape(str[i]);
    }
    _ostrm << '\'';
}

void AstPrinter::visitDoubleLiteralNode(DoubleLiteralNode* node) {
    _ostrm << node->literal();
}

void AstPrinter::visitIntLiteralNode(IntLiteralNode* node) {
    _ostrm << node->literal();
}

void AstPrinter::visitLoadNode(LoadNode* node) {
    _ostrm << node->var()->name();
}

void AstPrinter::visitStoreNode(StoreNode* node) {
    _ostrm << node->var()->name();
    _ostrm << kindToString(node->op());
    node->value()->visit(this);
}

void AstPrinter::visitForNode(ForNode* node) {
    _ostrm << "for (";
    if (node->var() == 0) {
        _ostrm << "undeclared_variable";
    } else {
        _ostrm << node->var()->name();
    }
    _ostrm << " in ";
    node->inExpr()->visit(this);
    _ostrm << ") ";
    node->body()->visit(this);
}

void AstPrinter::visitWhileNode(WhileNode* node) {
    _ostrm << "while (";
    node->whileExpr()->visit(this);
    _ostrm << ") ";
    node->loopBlock()->visit(this);
}

void AstPrinter::visitIfNode(IfNode* node) {
    _ostrm << "if (";
    node->ifExpr()->visit(this);
    _ostrm << ") ";
    node->thenBlock()->visit(this);
    _ostrm << " else ";
    node->elseBlock()->visit(this);
}

void AstPrinter::visitBlockNode(BlockNode* node) {
    _ostrm << "{\n";
    printFuncDecl(node->scope());
    printVarDecl(node->scope());
    printSubNodes(node);
    _ostrm << '}';
}

void AstPrinter::visitFunctionNode(FunctionNode* node) {
    _ostrm << "function " << typeToString(node->returnType());
    _ostrm << " " << node->name() << "(";
    if (node->parametersNumber() != 0) {
        _ostrm << node->parameterName(0);
    }
    for (uint32_t i = 1; i < node->parametersNumber(); ++i) {
        _ostrm << ", " << node->parameterName(i);
    }
    _ostrm << ") ";
    BlockNode* body = node->body();
    if (body->nodes() == 2 && body->nodeAt(0)->isNativeCallNode()) {
        body->nodeAt(0)->visit(this);
    } else {
        body->visit(this);
    }
}

void AstPrinter::visitReturnNode(ReturnNode* node) {
    _ostrm << "return ";
    node->visitChildren(this);
}

void AstPrinter::visitCallNode(CallNode* node) {
    _ostrm << node->name() << "(";
    if (node->parametersNumber() != 0) {
        node->parameterAt(0)->visit(this);
    }
    for (uint32_t i = 1; i < node->parametersNumber(); ++i) {
        _ostrm << ", ";
        node->parameterAt(i)->visit(this);
    }
    _ostrm << ')';
}

void AstPrinter::visitNativeCallNode(NativeCallNode* node) {
    // not sure how this is used. Need example.
    _ostrm << "native '" << node->nativeName() << "';";
}

void AstPrinter::visitPrintNode(PrintNode* node) {
    _ostrm << "print(";
    if (node->operands() != 0) {
        node->operandAt(0)->visit(this);
    }
    for (uint32_t i = 1; i < node->operands(); ++i) {
        _ostrm << ", ";
        node->operandAt(i)->visit(this);
    }
    _ostrm << ')';
}

void AstPrinter::printSubNodes(BlockNode* node) {
    for (uint32_t i = 0; i < node->nodes(); ++i) {
        AstNode* subNode = node->nodeAt(i);
        subNode->visit(this);
        if (!subNode->isIfNode() && !subNode->isWhileNode()
                && !subNode->isForNode()) {
            _ostrm << ';';
        }
        _ostrm << '\n';
    }
}

void AstPrinter::printFuncDecl(Scope* scope) {
    Scope::FunctionIterator it(scope);
    while (it.hasNext()) {
        it.next()->node()->visit(this);
        _ostrm << '\n';
    }
}

void AstPrinter::printVarDecl(Scope* scope) {
    Scope::VarIterator it(scope);
    while (it.hasNext()) {
        AstVar* var = it.next();
        _ostrm << typeToString(var->type()) << " " << var->name() << ";\n";
    }
}

} //namespace mathvm
