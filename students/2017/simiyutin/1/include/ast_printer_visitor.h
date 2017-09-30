#pragma once

#include <sstream>
#include "visitors.h"
#include "../../../../../include/visitors.h"
#include "../../../../../include/ast.h"


struct AstPrinterVisitor : AstBaseVisitor {

    void visitBinaryOpNode(BinaryOpNode * node) override {
        ss_ << '(';
        node->left()->visit(this);
        ss_ << ' ' << tokenOp(node->kind()) << ' ';
        node->right()->visit(this);
        ss_ << ')';
    }

    void visitUnaryOpNode(UnaryOpNode * node) override {
        ss_ << tokenOp(node->kind());
        node->visitChildren(this);
    }

    void visitStringLiteralNode(StringLiteralNode * node) override {
        ss_ << '\'' << escape(node->literal()) << '\'';
    }

    void visitDoubleLiteralNode(DoubleLiteralNode * node) override {
        ss_ << node->literal();
    }

    void visitIntLiteralNode(IntLiteralNode * node) override {
        ss_ << node->literal();
    }

    void visitLoadNode(LoadNode * node) override {
        ss_ << node->var()->name();
    }

    void visitStoreNode(StoreNode * node) override {
        indent(node->var()->name());
        ss_ << ' ' << tokenOp(node->op()) << ' ';
        node->value()->visit(this);
        ss_ << ";";
        newline();
    }

    void visitForNode(ForNode * node) override {
        indent("for ("); ss_ << node->var()->name() << " in "; node->inExpr()->visit(this); ss_ << ") {";
        newline();
            tab();
            node->body()->visit(this);
            untab();
        indent("}");
        newline();
    }

    void visitWhileNode(WhileNode * node) override {
        indent("while ("); node->whileExpr()->visit(this); ss_ << ") {";
        newline();
            tab();
            node->loopBlock()->visit(this);
            untab();
        indent("}");
        newline();
    }

    void visitIfNode(IfNode * node) override    {

        indent("if ("); node->ifExpr()->visit(this); ss_ << ") {";
        newline();
            tab();
            node->thenBlock()->visit(this);
            untab();
        indent("}");
        if (node->elseBlock()) {
            ss_ << " else {";
            newline();
                tab();
                node->elseBlock()->visit(this);
                untab();
            indent("}");
        }
        newline();
    }

    void visitBlockNode(BlockNode * node) override {

        Scope::VarIterator it(node->scope());

        while (it.hasNext()) {
            const AstVar * var = it.next();
            indent(type_str(var->type())); ss_ << ' ' << var->name() << ';';
            newline();
        }

        Scope::FunctionIterator fit(node->scope());
        while (fit.hasNext()) {
            const AstFunction * func = fit.next();
            indent("function ");
            ss_ << type_str(func->returnType())
               << ' '
               << func->name()
               << '(';
            for (size_t i = 0; i < func->parametersNumber(); ++i) {
                ss_ << type_str(func->parameterType(i)) << ' ' << func->parameterName(i);
                if (i < func->parametersNumber() - 1) {
                    ss_ << ", ";
                }
            }
            ss_ << ") ";
            func->node()->visit(this);
        }

        node->visitChildren(this);
    }

    void visitFunctionNode(FunctionNode * node) override {
        NativeCallNode * native = check_native(node);
        if (native) {
            native->visit(this);
        } else {
            ss_ << "{";
            newline();
            tab();
            node->visitChildren(this);
            untab();
            indent("}");
            newline();
        }
    }

    void visitReturnNode(ReturnNode * node) override {
        indent("return");
        if (node->returnExpr()) {
            ss_ << ' ';
            node->returnExpr()->visit(this);
        }
        ss_ << ";";
        newline();
    }

    void visitCallNode(CallNode * node) override {
        if (need_indent_) {
            indent("");
            ss_ << node->name();
            print_parameters(node);
            ss_ << ';';
            newline();
        } else {
            ss_ << node->name();
            print_parameters(node);
        }

    }

    void visitNativeCallNode(NativeCallNode * node) override {
        ss_ << "native '" << node->nativeName()<< "';";
        newline();
    }

    void visitPrintNode(PrintNode * node) override {
        indent("print(");
        for (int i = 0; i < (int) node->operands(); ++i) {
            node->operandAt(i)->visit(this);
            if (i < int (node->operands()) - 1) {
                ss_ << ", ";
            }
        }
        ss_ << ");";
        newline();
    }

    const string get_program() const {
        return ss_.str();
    }

private:

    template <typename T>
    void print_parameters(const T * node) {
        ss_ << '(';
        for (int i = 0; i < (int) node->parametersNumber(); ++i) {
            node->parameterAt(i)->visit(this);
            if (i < int (node->parametersNumber()) - 1) {
                ss_ << ", ";
            }
        }
        ss_ << ')';
    }

    template <typename T>
    void indent(const T &el) {
        for (int i = 0; i < indent_; ++i) {
            ss_ << ' ';
        }
        need_indent_ = false;
        ss_ << el;
    }

    void newline() {
        ss_ << '\n';
        need_indent_ = true;
    }

    void tab() {
        indent_ += 4;
    }

    void untab() {
        indent_ -= 4;
    }

    string escape_char(char c) {
        switch (c) {
            case '\n':
                return "\\n";
            case '\'':
                return "\\'";
            case '\\':
                return "\\\\";
            case '\r':
                return "\\r";
            case '\t':
                return "\\t";
            case '\v':
                return "\\v";
            default:
                return to_string(c);
        }
    }

    string escape(const string & str) {
        stringstream res;
        for (size_t i = 0; i < str.size(); ++i) {
            char c = str[i];
            if (isprint(c)) {
                res << c;
            } else {
                res << escape_char(c);
            }
        }
        return res.str();
    }

    string type_str(VarType t) {
        vector<string> types = {"<invalid>", "void", "double", "int", "string"};
        return types[(int) t];
    }

    NativeCallNode * check_native(FunctionNode * node) {
        BlockNode * block = node->body();
        AstNode * first_child = block->nodeAt(0);
        NativeCallNode * native = dynamic_cast<NativeCallNode*>(first_child);
        return native;
    }

    stringstream ss_;
    int indent_ = 0;
    bool need_indent_ = true;
};