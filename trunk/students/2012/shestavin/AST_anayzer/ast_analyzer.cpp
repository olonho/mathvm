//
//  ast_analyzer.cpp
//
//  Created by Dmitriy on 9/27/12.
//  Copyright (c) 2012 Dmitriy. All rights reserved.
//

#include "ast_analyzer.h"

string ASTAnalyzer::escape(const string& s) const {
	string result;
	for (string::const_iterator i = s.begin(); i != s.end(); ++i) {
		switch (*i) {
            case '\n':
                result += "\\n";
                break;
            case '\r':
                result += "\\r";
                break;
            case '\\':
                result += "\\\\";
                break;
            case '\t':
                result += "\\t";
                break;
            default:
                result += *i;
                break;
		}
	}
	return result;
}

void ASTAnalyzer::visitBlockNode (BlockNode* node) {
    output << "{\n";
    printScopeDeclarations(node->scope());
    printBlock(node);
    output << "}\n";
}

void ASTAnalyzer::printScopeDeclarations (Scope* scope) {
    Scope::FunctionIterator fuctions(scope);
    while (fuctions.hasNext()) {
        fuctions.next()->node()->visit(this);
    }
    Scope::VarIterator vars(scope);
    while (vars.hasNext()) {
        AstVar* var = vars.next();
        output << typeToName(var->type()) << " " << var->name() << ";\n";
    }
}

void ASTAnalyzer::printBlock (BlockNode* node) {
    for (unsigned int i = 0; i < node->nodes(); ++i) {
        AstNode* innerNode = node->nodeAt(i);
        innerNode->visit(this);
        if (!innerNode->isIfNode() && !innerNode->isWhileNode()
            && !innerNode->isForNode()) {
            output << ';';
        }
        output << '\n';
    }
}

void ASTAnalyzer::visitBinaryOpNode(BinaryOpNode* node) {
    output << '(';
    node->left()->visit(this);
    output << ' ' << tokenOp(node->kind()) << ' ';
    node->right()->visit(this);
    output << ')';
}

void ASTAnalyzer::visitUnaryOpNode(UnaryOpNode* node) {
    output << tokenOp(node->kind());
    node->operand()->visit(this);
}

void ASTAnalyzer::visitStringLiteralNode(StringLiteralNode* node) {
    output << '\'' << escape(node->literal()) << '\'';
}

void ASTAnalyzer::visitDoubleLiteralNode(DoubleLiteralNode* node) {
    output << node->literal();
}

void ASTAnalyzer::visitIntLiteralNode(IntLiteralNode* node) {
    output << node->literal();
}

void ASTAnalyzer::visitLoadNode(LoadNode* node) {
    output << node->var()->name();
}

void ASTAnalyzer::visitStoreNode(StoreNode* node) {
    output << node->var()->name();
    output << ' ' << tokenOp(node->op()) << ' ';
    node->value()->visit(this);
}

void ASTAnalyzer::visitForNode(ForNode* node) {
    output << "for (" << node->var()->name() << " in ";
    node->inExpr()->visit(this);
    output << ')';
    node->body()->visit(this);
}

void ASTAnalyzer::visitWhileNode(WhileNode* node) {
    output << "while (";
    node->whileExpr()->visit(this);
    output << ')';
    node->loopBlock()->visit(this);
}

void ASTAnalyzer::visitIfNode(IfNode* node) {
    output << "if (";
    node->ifExpr()->visit(this);
    output << ')';
    node->thenBlock()->visit(this);

    if(node->elseBlock()) {
        output << " else ";
        node->elseBlock()->visit(this);
    }
}


void ASTAnalyzer::visitFunctionNode(FunctionNode* node) {
    output << "function " << typeToName(node->returnType()) << ' ' << node->name();
    output << '(';
    for (size_t i = 0; i < node->parametersNumber(); ++i) {
        output << typeToName(node->parameterType(i)) << ' ' << node->parameterName(i);
        if (i != node->parametersNumber() - 1) {
            output << ", ";
        }
    }
    output << ')';

    if (node->body()->nodeAt(0)->isNativeCallNode()) {
        node->body()->nodeAt(0)->visit(this);
    } else {
        node->body()->visit(this);
    }
}

void ASTAnalyzer::visitReturnNode(ReturnNode* node) {
    output << "return ";
    node->visitChildren(this);
}

void ASTAnalyzer::visitCallNode( CallNode* node ) {
    output << node->name() ;
    output << '(';
    for(size_t i = 0; i < node->parametersNumber(); ++i) {
        node->parameterAt(i)->visit(this);
        if (i != node->parametersNumber() - 1) {
            output << ", ";
        }
    }
    output << ')';
}

void ASTAnalyzer::visitNativeCallNode(NativeCallNode* node) {
    output << " native '" << node->nativeName() << "';";
}

void ASTAnalyzer::visitPrintNode(PrintNode* node) {
    output << "print(" ;
    for (size_t i = 0; i < node->operands(); ++i) {
        node->operandAt(i)->visit(this);
        if (i != node->operands() - 1) {
            output << ", ";
        }
    }
    output << ')';
}