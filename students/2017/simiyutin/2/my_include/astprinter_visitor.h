#pragma once

#include "includes.h"

#include <sstream>

struct AstPrinterVisitor : mathvm::AstBaseVisitor {

#define VISITOR_FUNCTION(type, name) \
    virtual void visit##type(mathvm::type* node) override;

    FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION

    const std::string get_program() const;

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

    void newline();

    void tab();

    void untab();

    std::string escape_char(char c);

    std::string escape(const std::string & str);

    std::string type_str(mathvm::VarType t);

    mathvm::NativeCallNode * check_native(mathvm::FunctionNode * node);

    std::stringstream ss_;
    int indent_ = 0;
    bool need_indent_ = true;
};