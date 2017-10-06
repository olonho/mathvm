#ifndef _AST_PRINTER_IMPL_H
#define _AST_PRINTER_IMPL_H

#include "../include/mathvm.h"
#include "../include/ast.h"

#include <sstream>

namespace mathvm {

class AstPrinter : public AstVisitor {
    std::stringstream pout;
    bool needSemicolon;
public:
    AstPrinter(): pout(), needSemicolon(true) {}

    string program() {
        return pout.str();
    }

    virtual void visitBinaryOpNode(BinaryOpNode* node) {
        node->left()->visit(this);
        pout << " " << tokenOp(node->kind()) << " ";
        node->right()->visit(this);
    }

    virtual void visitUnaryOpNode(UnaryOpNode* node) {
        pout << tokenOp(node->kind());
        node->operand()->visit(this);
    }

    virtual void visitStringLiteralNode(StringLiteralNode* node) {
        pout << node->literal();
    }

    virtual void visitDoubleLiteralNode(DoubleLiteralNode* node) {
        pout << node->literal();
    }

    virtual void visitIntLiteralNode(IntLiteralNode* node) {
        pout << node->literal();
    }

    virtual void visitLoadNode(LoadNode* node) {
        const AstVar* var = node->var();
        pout << var->name();
    }

    virtual void visitStoreNode(StoreNode* node) {
        pout << node->var()->name() << " "
                  << tokenOp(node->op()) << " ";
        node->value()->visit(this);
    }

    virtual void visitForNode(ForNode* node) {
        pout << "for (" << node->var()->name() << " in ";
        node->inExpr()->visit(this);
        pout << "( {\n";
        node->body()->visit(this);
        pout << "}\n";
        needSemicolon = false;
    }

    virtual void visitWhileNode(WhileNode* node) {
        pout << "while(";
        node->whileExpr()->visit(this);
        pout << ") {\n";
        node->loopBlock()->visit(this);
        pout << "}\n";
        needSemicolon = false;
    }

    virtual void visitIfNode(IfNode* node) {
        pout << "if (";
        node->ifExpr()->visit(this);
        pout << ") {\n";
        node->thenBlock()->visit(this);
        pout << "}";
        BlockNode* elseBlock = node->elseBlock();
        if (elseBlock) {
            pout << " else (";
            elseBlock->visit(this);
            pout << "}";
        }
        pout << "\n";
        needSemicolon = false;
    }

    virtual void visitBlockNode(BlockNode* node) {
        for (uint32_t i = 0; i < node->nodes(); i++) {
            node->nodeAt(i)->visit(this);
            if(needSemicolon) {
                pout << ";\n";
            } else {
                needSemicolon = true;
            }
        }
    }

    virtual void visitFunctionNode(FunctionNode* node) {
        string returnType =
                string(typeToName(node->returnType()));
        pout << "function " << returnType << " (";
        for(uint32_t i = 0; i < node->parametersNumber(); i++) {
            if(i > 0) {
                pout << ", ";
            }

            string parameterType =
                    string(typeToName(node->parameterType(i)));
            string parameterName = node->parameterName(i);
            pout << parameterType << " " << parameterName;
        }
        pout << ") {\n";
        node->body()->visit(this);
        pout << "}\n";
        needSemicolon = false;
    }

    virtual void visitReturnNode(ReturnNode* node) {
        pout << "return ";
        node->returnExpr()->visit(this);
    }

    virtual void visitCallNode(CallNode* node) {
        pout << node->name() << "(";
        for(uint32_t i = 0; i < node->parametersNumber(); i++) {
            if(i > 0) {
                pout << ", ";
            }
            node->parameterAt(i)->visit(this);
        }
        pout << ")";
    }

    virtual void visitNativeCallNode(NativeCallNode* node) {
        pout << "native '" << node->nativeName() << "'";
    }

    virtual void visitPrintNode(PrintNode* node) {
        pout << "print(";
        for(uint32_t i = 0; i < node->operands(); i++) {
            if(i > 0) {
                pout << ", ";
            }
            node->operandAt(i)->visit(this);
        }
        pout << ")";
    }

};

}

#endif // _AST_PRINTER_IMPL_H
