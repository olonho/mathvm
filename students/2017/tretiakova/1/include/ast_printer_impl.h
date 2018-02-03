#ifndef _AST_PRINTER_IMPL_H
#define _AST_PRINTER_IMPL_H

#include "../include/mathvm.h"
#include "../include/ast.h"

#include <sstream>

namespace mathvm {

class AstPrinter : public AstVisitor {
    std::stringstream pout;
    bool need_semicolon;
    int indent_size;
    string indent;

    const char indent_char = ' ';
    const int indent_shift = 4;
    string create_indent(int size) {
        indent = "";
        for(int i = 0; i < size; ++i) {
            indent += ' ';
        }
        return indent;
    }

public:
    AstPrinter(): pout(), need_semicolon(true),
        indent_size(-4), indent("") {}

    string program() {
        return pout.str();
    }

    virtual void visitBinaryOpNode(BinaryOpNode* node) {
        cerr << "[BinaryOp]" << endl;

        node->left()->visit(this);
        pout << " " << tokenOp(node->kind()) << " ";
        node->right()->visit(this);
    }

    virtual void visitUnaryOpNode(UnaryOpNode* node) {
        cerr << "[UnaryOp]" << endl;

        pout << tokenOp(node->kind());
        node->operand()->visit(this);
    }

    virtual void visitStringLiteralNode(StringLiteralNode* node) {
        cerr << "[StringLiteral]" << endl;

        string literal = node->literal();
        string new_literal = "";
        for(int i = 0; i < (int)literal.size(); ++i) {
            if(literal[i] != '\n') {
                new_literal += literal[i];
            } else {
                new_literal += "\\n";
            }
        }
        pout << "'" << new_literal << "'";
    }

    virtual void visitDoubleLiteralNode(DoubleLiteralNode* node) {
        cerr << "printer [DoubleLiteral]" << node->literal() << endl;

        pout << node->literal();
    }

    virtual void visitIntLiteralNode(IntLiteralNode* node) {
        cerr << "[IntLiteral]" << endl;

        pout << node->literal();
    }

    virtual void visitLoadNode(LoadNode* node) {
        cerr << "[Load]" << endl;

        const AstVar* var = node->var();
        pout << var->name();
    }

    virtual void visitStoreNode(StoreNode* node) {
        cerr << "[Store]" << endl;

        pout << node->var()->name() << " "
                  << tokenOp(node->op()) << " ";
        node->value()->visit(this);
    }

    virtual void visitForNode(ForNode* node) {
        cerr << "[For]" << endl;

        pout << "for (" << node->var()->name() << " in ";
        node->inExpr()->visit(this);
        pout << ") {\n";
        node->body()->visit(this);
        pout << indent << "}\n";
        need_semicolon = false;
    }

    virtual void visitWhileNode(WhileNode* node) {
        cerr << "[While]" << endl;

        pout << "while(";
        node->whileExpr()->visit(this);
        pout << ") {\n";
        node->loopBlock()->visit(this);
        pout << create_indent(indent_size - indent_shift) << "}\n";
        need_semicolon = false;
    }

    virtual void visitIfNode(IfNode* node) {
        cerr << "[If]" << endl;

        pout << "if (";
        node->ifExpr()->visit(this);
        pout << ") {\n";
        node->thenBlock()->visit(this);
        cerr << "[IF indent_size " << indent_size << "]" << endl;
        pout << indent << "}";
        BlockNode* elseBlock = node->elseBlock();
        if (elseBlock) {
            pout << " else {";
            elseBlock->visit(this);
            pout << indent << "}";
        }
        pout << "\n";
        need_semicolon = false;
    }

    virtual void visitBlockNode(BlockNode* node) {
        cerr << "[Block]" << endl;

        indent_size += indent_shift;
        cerr << "[" << indent_size << " " << node << "]" << endl;
        create_indent(indent_size);

        for(Scope::VarIterator it(node->scope()); it.hasNext();) {
            AstVar* var = it.next();
            pout << indent << typeToName(var->type()) << " "
                 << var->name() << ";\n";
        }

        for(Scope::FunctionIterator it(node->scope()); it.hasNext();) {
            AstFunction* fun = it.next();
            fun->node()->visit(this);
            need_semicolon = true;
        }

        for (uint32_t i = 0; i < node->nodes(); i++) {
            pout << indent;

            node->nodeAt(i)->visit(this);
            if(need_semicolon) {
                pout << ";\n";
            } else {
                need_semicolon = true;
            }
        }

        indent_size -= indent_shift;
        create_indent(indent_size);
        cerr << "[/" << indent_size << " " << node << "]" << endl;
    }

    virtual void visitFunctionNode(FunctionNode* node) {
        cerr << "[Function]" << node->name() << endl;

        pout << "function " << typeToName(node->returnType())
             << " " << node->name() << "(";
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
        pout << create_indent(indent_size - indent_shift) << "}\n";
        need_semicolon = false;
    }

    virtual void visitReturnNode(ReturnNode* node) {
        cerr << "[Return]" << endl;

        pout << "return ";
        AstNode* return_expr = node->returnExpr();
        if(return_expr == NULL) {
            pout << "void";
        } else {
            return_expr->visit(this);
        }
    }

    virtual void visitCallNode(CallNode* node) {
        cerr << "[Call]" << endl;

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
        cerr << "[NativeCall]" << endl;

        pout << "native '" << node->nativeName() << "'";
    }

    virtual void visitPrintNode(PrintNode* node) {
        cerr << "[Print]" << endl;

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
