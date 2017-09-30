#pragma once

#include <sstream>
#include "visitors.h"
#include "../../../../../include/visitors.h"
#include "../../../../../include/ast.h"


struct AstPrinterVisitor : AstBaseVisitor {

    void visitBinaryOpNode(BinaryOpNode * node) override {
        ss << '(';
        node->left()->visit(this);
        ss << ' ' << tokenOp(node->kind()) << ' ';
        node->right()->visit(this);
        ss << ')';
    }

    void visitUnaryOpNode(UnaryOpNode * node) override {
        ss << tokenOp(node->kind());
        node->visitChildren(this);
    }

    void visitStringLiteralNode(StringLiteralNode * node) override {
        ss << '\'' << escape(node->literal()) << '\'';
        node->visitChildren(this);
    }

    void visitDoubleLiteralNode(DoubleLiteralNode * node) override {
        ss << node->literal();
        node->visitChildren(this);
    }

    void visitIntLiteralNode(IntLiteralNode * node) override {
        ss << node->literal();
        node->visitChildren(this);
    }

    void visitLoadNode(LoadNode * node) override {
        ss << node->var()->name();
        node->visitChildren(this);
    }

    void visitStoreNode(StoreNode * node) override {
        pi(node->var()->name());
        ss << ' ' << tokenOp(node->op()) << ' ';
        node->value()->visit(this);
        ss << ";";
        ip();
    }

    void visitForNode(ForNode * node) override {
        pi("for ("); ss << node->var()->name() << " in "; node->inExpr()->visit(this); ss << ") {"; ip();
            tab();
            node->body()->visit(this);
            untab();
        pi("}"); ip();
    }

    void visitWhileNode(WhileNode * node) override { //todo
        pi("while ("); node->whileExpr()->visit(this); ss << ") {"; ip();
            tab();
            node->loopBlock()->visit(this);
            untab();
        pi("}"); ip();
    }

    void visitIfNode(IfNode * node) override    {

        pi("if ("); node->ifExpr()->visit(this); p(") {"); ip();
            tab();
            node->thenBlock()->visit(this);
            untab();
        pi("}");
        if (node->elseBlock()) {
            p(" else {"); ip();
                tab();
                node->elseBlock()->visit(this);
                untab();
            pi("}");
        }
        ip();
    }

    void visitBlockNode(BlockNode * node) override {

        Scope::VarIterator it(node->scope());

        while (it.hasNext()) {
            const AstVar * var = it.next();
            pi(type_str(var->type())); p(' '); p(var->name()); p(";"); ip();
        }

        Scope::FunctionIterator fit(node->scope());
        while (fit.hasNext()) {
            const AstFunction * func = fit.next();
            pi("function ");
            p(type_str(func->returnType()));
            p(' ');
            p(func->name());
            p('(');
            for (size_t i = 0; i < func->parametersNumber(); ++i) {
                ss << type_str(func->parameterType(i)) << ' ' << func->parameterName(i);
                if (i < func->parametersNumber() - 1) {
                    ss << ", ";
                }
            }
            ss << ") {"; ip();
                tab();
                func->node()->visit(this);
                untab();
            pi("}"); ip();
        }

        node->visitChildren(this);
    }

    void visitFunctionNode(FunctionNode * node) override {
        node->visitChildren(this);
    }

    void visitReturnNode(ReturnNode * node) override {
        pi("return");
        if (node->returnExpr()) {
            ss << ' ';
            node->returnExpr()->visit(this);
        }
        ss << ";"; ip();
    }

    void visitCallNode(CallNode * node) override {
        if (is_newline) {
            pi("");
            p(node->name());// todo smart indent
            print_parameters(node);
            ss << ';';
            ip();
        } else {
            p(node->name());// todo smart indent
            print_parameters(node);
        }

    }

    void visitNativeCallNode(NativeCallNode * node) override {
        ss << "NativeCallNode!!!!" << std::endl;
        node->visitChildren(this);
    } //todo

    void visitPrintNode(PrintNode * node) override {
        pi("print(");
        for (int i = 0; i < (int) node->operands(); ++i) {
            node->operandAt(i)->visit(this);
            if (i < int (node->operands()) - 1) {
                p(", ");
            }
        }
        p(");"); ip();
    }

    const string get_program() const {
        return ss.str();
    }

private:

    template <typename T>
    void print_parameters(const T * node) {
        p("(");
        for (int i = 0; i < (int) node->parametersNumber(); ++i) {
            node->parameterAt(i)->visit(this);
            if (i < int (node->parametersNumber()) - 1) {
                p(", ");
            }
        }
        p(')');
    }

    template <typename T>
    void pi(const T &el) {
        for (int i = 0; i < indent; ++i) {
            ss << ' ';
        }
        is_newline = false;
        p(el);
    }

    void ip() {
        ss << '\n';
        is_newline = true;
    }

    template <typename T>
    void p(const T &el) {
        ss << el;
    }

    void tab() {
        indent += 4;
    }

    void untab() {
        indent -= 4;
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

    stringstream ss;
    int indent = 0;
    bool is_newline = true;

};