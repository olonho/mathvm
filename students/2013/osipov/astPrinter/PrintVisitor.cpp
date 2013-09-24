/* 
 * File:   PrintVisitor.cpp
 * Author: stasstels
 * 
 * Created on September 23, 2013, 3:20 PM
 */

#include <algorithm>

#include "PrintVisitor.h"

namespace mathvm {
    
    const uint32_t PrintVisitor::TAB_SIZE = 4;
    
    const char PrintVisitor::SPACE = ' ';
    
    PrintVisitor::PrintVisitor(std::ostream& out) : tab_level(0), out(out) {
    }
    
    void PrintVisitor::visitBlockNodeInside(BlockNode* node) {
        printVariableDeclarations(node -> scope());
        printFunctions(node -> scope());
        std::string tab(tab_level * TAB_SIZE, SPACE);
        for(uint32_t i = 0; i < node -> nodes(); ++i) {
            out << tab;
            node -> nodeAt(i) -> visit(this); 
            out << ";" << std::endl;
        }
    }
    
    bool PrintVisitor::hasParens(TokenKind currentOperator, AstNode* expr) {
        if(currentOperator == tRANGE) {
            return true;
        }
//        UnaryOpNode* uOp = dynamic_cast<UnaryOpNode*>(expr);
//        if(uOp != 0) {
//            return tokenPrecedence(currentOperator) > tokenPrecedence(uOp -> kind());
//        }
        BinaryOpNode* bOp = dynamic_cast<BinaryOpNode*>(expr);
        if(bOp != 0) {
            return tokenPrecedence(currentOperator) > tokenPrecedence(bOp -> kind());
        }
        return false;
    }
    
    void PrintVisitor::visitBinaryOpNode(BinaryOpNode * node) {
        bool leftParens = hasParens(node->kind(), node->left());
        bool rightParens = hasParens(node->kind(), node->right());
        out << (leftParens ? "(" : "");
        node->left()->visit(this);
        out << (leftParens ? ")" : "");
        out << " " << tokenOp(node->kind()) << " ";
        out << (rightParens ? "(" : "");
        node->right() -> visit(this);
        out << (rightParens ? ")" : "");
    }

    void PrintVisitor::visitUnaryOpNode(UnaryOpNode* node) {
        out << tokenOp(node->kind());
        node->operand()->visit(this);
    }
    
    void PrintVisitor::visitStringLiteralNode(StringLiteralNode * node) {
        out << "'";
        for(auto c : node -> literal()) {
            switch(c) {
                case '\n':
                    out << "\\n";
                    break;
                case '\t':
                    out << "\\t";
                    break;
                case '\f':
                    out << "\\f";
                    break;
                case '\r':
                    out << "\\r";
                    break;
                default:
                    out << c;
            }
        }
        out << "'";
    }
    
    void PrintVisitor::visitDoubleLiteralNode(DoubleLiteralNode * node) {
        out << std::fixed << node -> literal();
    } 
    
    void PrintVisitor::visitIntLiteralNode(IntLiteralNode * node) {
        out << node -> literal();
    } 
    
    void PrintVisitor::visitLoadNode(LoadNode * node) {
        out << node -> var() -> name();
    } 
    
    
    void PrintVisitor::visitStoreNode(StoreNode * node) {
        out << node -> var() -> name() 
                << " "
                << tokenOp(node->op())
                << " ";
        node -> visitChildren(this);
    }
    
    
    void PrintVisitor::visitForNode(ForNode * node) {
        out << "for (" << node->var()->name() << " in ";
        node->inExpr()->visit(this);
        out << ") ";
        node->body()->visit(this);
    }
    
    void PrintVisitor::visitWhileNode(WhileNode * node) {
        out << "while (";
        node->whileExpr()->visit(this);
        out << ") ";
        node->loopBlock()->visit(this);
    }
    
    void PrintVisitor::visitIfNode(IfNode * node) {
        out << "if (";
        node -> ifExpr() -> visit(this);
        out << ") ";
        node -> thenBlock() -> visit(this);
        if(node->elseBlock()) {
            out << " else";
            node -> elseBlock() -> visit(this);
        }
    } 
    
    void PrintVisitor::visitBlockNode(BlockNode* node) {
        out << "{" << std::endl;
        ++tab_level;
        visitBlockNodeInside(node);
        if(tab_level != 0) {
            out << std::string(--tab_level * TAB_SIZE, SPACE);
        }
        out << "}" << std::endl;
    } 
    
    void PrintVisitor::visitFunctionNode(FunctionNode* node) {
        out << "function " << typeToName(node->returnType()) << " " << node->name() << "(";
        for(uint32_t i = 0; i < node->parametersNumber(); ++i) {
            if(i != 0) {
                out << ", ";
            }
            out << typeToName(node->parameterType(i)) << " " << node->parameterName(i);
        }
        out << ")";
        if(node->body()->nodeAt(0)->isNativeCallNode()) {
            node->body()->nodeAt(0)->visit(this);
        } else {
            node->body()->visit(this);
        }
        out << std::endl;
    }
    
    void PrintVisitor::visitReturnNode(ReturnNode * node) {
        out << "return";
        if(node->returnExpr()) {
            out << " ";
            node->returnExpr()->visit(this);
        }
    }
    
    void PrintVisitor::visitCallNode(CallNode * node) {
        out << node->name() << "(";
        for(uint32_t i = 0; i < node->parametersNumber(); ++i) {
            if(i != 0) {
                out << ", ";
            }
            node->parameterAt(i)->visit(this);
        }
        out << ")";
    } 
    void PrintVisitor::visitNativeCallNode(NativeCallNode * node) {
        out << " native '" << node->nativeName() << "';";
    }
    
    void PrintVisitor::visitPrintNode(PrintNode * node) {
        out << "print(";
        for(uint32_t i = 0; i < node->operands(); ++i) {
            if(i != 0) {
                out << ",";
            }
            node -> operandAt(i) -> visit(this);
        }
        out << ")";
    }
    

}


 
    