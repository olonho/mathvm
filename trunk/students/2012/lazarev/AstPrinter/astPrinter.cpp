#include "astPrinter.h"
#include <iostream>
    
string AstPrinter::opToString(TokenKind tokenKind) {
    switch (tokenKind) {
        #define ENUM_ELEM(t, s, p) case t: return s;
            FOR_TOKENS(ENUM_ELEM)
        #undef ENUM_ELEM
            default: return "";
    }
}

void AstPrinter::visitBinaryOpNode(BinaryOpNode* node) {
    std::cout << "(";
    node->left()->visit(this);
    std::cout << opToString(node->kind());
    node->right()->visit(this);
    std::cout << ")";
    blockEnded = false;
}

void AstPrinter::visitUnaryOpNode(UnaryOpNode* node) {
    std::cout << "(" << opToString(node->kind());
    node->operand()->visit(this);
    std::cout << ")";
    blockEnded = false;
}

void AstPrinter::visitStringLiteralNode(StringLiteralNode* node) {
    std::cout << "'";
    std::string s = node->literal();
    for (unsigned int i = 0; i < s.length(); i++) {
        if (s[i] == '\'') {
            std::cout << "\\'";
        } else if (s[i] == '\\') {
            std::cout << "\\\\";
        } else if (s[i] == '\t') {
            std::cout << "\\t";
        } else if (s[i] == '\n') {
            std::cout << "\\n";
        } else {
            std::cout << s[i];
        }
    }
    std::cout << "'";
    blockEnded = false;
}

void AstPrinter::visitDoubleLiteralNode(DoubleLiteralNode* node) {
    std::cout << node->literal();
    blockEnded = false;
}

void AstPrinter::visitIntLiteralNode(IntLiteralNode* node) {
    std::cout << node->literal();
    blockEnded = false;
}

void AstPrinter::visitLoadNode(LoadNode* node) {
    std::cout << node->var()->name();
    blockEnded = false;
}

void AstPrinter::visitStoreNode(StoreNode* node) {
    std::cout << node->var()->name() << " " << opToString(node->op()) << " ";
    node->value()->visit(this);
    blockEnded = false;
}

void AstPrinter::visitForNode(ForNode* node) {
    std::cout << "for (" << node->var()->name() << " in ";
    node->inExpr()->visit(this);
    std::cout << ") {" << std::endl;
    node->body()->visit(this);
    std::cout << "}" << std::endl;
    blockEnded = true;
}

void AstPrinter::visitWhileNode(WhileNode* node) {
    std::cout << "while (";
    node->whileExpr()->visit(this);
    std::cout << ") {" << std::endl;
    node->loopBlock()->visit(this);
    std::cout << "}" << std::endl;
    blockEnded = true;
}

void AstPrinter::visitIfNode(IfNode* node) {
    std::cout << "if (";
    node->ifExpr()->visit(this);
    std::cout << ") {" << std::endl;
    node->thenBlock()->visit(this);
    std::cout << "}";

    if (node->elseBlock() != 0) {
        std::cout << " else {" << std::endl;
        node->elseBlock()->visit(this);
        std::cout << "}";
    }
    std::cout << std::endl;
    blockEnded = true;
}

void AstPrinter::visitBlockNode(BlockNode* node) {
    Scope::VarIterator vars(node->scope());
    while (vars.hasNext()) {
        AstVar* var = vars.next();
        std::cout << typeToName(var->type()) << " " << var->name() << ";" << std::endl;
    }

    Scope::FunctionIterator functions(node->scope());
    while (functions.hasNext()) {
        functions.next()->node()->visit(this);
    }

    for (unsigned int i = 0; i < node->nodes(); i++) {
        node->nodeAt(i)->visit(this);
        if (!blockEnded) {
            std::cout << ";" << std::endl;
        }
    }
    blockEnded = false;
}

void AstPrinter::visitFunctionNode(FunctionNode* node) {
    std::cout << "function " << typeToName(node->returnType()) << " " << node->name() << "(";
    for (unsigned int i = 0; i < node->parametersNumber(); i++) {
        std::cout << typeToName(node->parameterType(i)) << " " << node->parameterName(i);
        if (i != node->parametersNumber() - 1) {
            std::cout << ", ";
        }
    }
    std::cout << ") ";
    if (node->body()->nodeAt(0)->isNativeCallNode()) {
        node->body()->nodeAt(0)->visit(this);
        std::cout << ";" << std::endl;
        blockEnded = false;
    } else {
        std::cout << "{" << std::endl;
        node->body()->visit(this);
        std::cout << "}" << std::endl;
        blockEnded = true;
    }
}

void AstPrinter::visitReturnNode(ReturnNode* node) {
    std::cout << "return";
    if (node->returnExpr() != 0) {
        std::cout << " ";
        node->returnExpr()->visit(this);
    }
    blockEnded = false;
}

void AstPrinter::visitCallNode(CallNode* node) {
    std::cout << node->name() << "(";
    for (unsigned int i = 0; i < node->parametersNumber(); i++) {
        node->parameterAt(i)->visit(this);
        if (i != node->parametersNumber() - 1) {
            std::cout << ", ";
        }
    }
    std::cout << ")";
    blockEnded = false;
}

void AstPrinter::visitNativeCallNode(NativeCallNode* node) {
    std::cout << "native '" << node->nativeName() << "'";
    blockEnded = false;
}

void AstPrinter::visitPrintNode(PrintNode* node) {
    std::cout << "print(";
    for (unsigned int i = 0; i < node->operands(); i++) {
        node->operandAt(i)->visit(this);
        if (i != node->operands() - 1) {
            std::cout << ", ";
        }
    }
    std::cout << ")";
    blockEnded = false;
}