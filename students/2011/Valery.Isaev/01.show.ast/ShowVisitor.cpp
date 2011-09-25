#include <iostream>
#include <sstream>

#include "ShowVisitor.h"

const int tab = 4;

#define DO_CASE_OP(t, s, u) \
    case mathvm::t: op = s; break;

#define SHOW_OP(f) ({ \
    const char* op = ""; \
    switch (node->f()) { \
        FOR_TOKENS(DO_CASE_OP) \
        case mathvm::tTokenCount:; \
    } \
    op; })

#define DO_CASE_PREC(t, s, u) \
    case mathvm::t: prec = u; break;

#define GET_PREC(f) ({ \
    int prec = 0; \
    switch (node->f()) { \
        FOR_TOKENS(DO_CASE_PREC) \
        case mathvm::tTokenCount:; \
    } \
    prec; })

#define SHOW_TYPE(f) ({ \
    const char* typ = ""; \
    switch (f) { \
        case mathvm::VT_DOUBLE: typ = "double"; break; \
        case mathvm::VT_INT: typ = "int"; break; \
        case mathvm::VT_STRING: typ = "string"; break; \
        case mathvm::VT_INVALID: typ = ""; \
    } \
    typ; })

#define TABS \
    if (need_tabs) { \
        stream << std::string(level * tab, ' '); \
        need_tabs = false; \
    }

#define ENTER(c, x) { \
    stream << (c ? ") {\n" : "{\n"); \
    ++level; \
    need_tabs = true; \
    node->x()->visit(this); \
    --level; \
    stream << "}"; \
}

ShowVisitor::ShowVisitor(std::ostream& o): need_tabs(false),
    prec(0), level(0), stream(o) {}

void ShowVisitor::visitBinaryOpNode(mathvm::BinaryOpNode* node) {
    TABS
    int pprec = prec;
    prec = GET_PREC(kind);
    if (pprec > prec) stream << '(';
    node->left()->visit(this);
    stream << " " << SHOW_OP(kind) << ' ';
    ++prec;
    node->right()->visit(this);
    if (pprec >= prec) stream << ')';
    prec = pprec;
}

void ShowVisitor::visitUnaryOpNode(mathvm::UnaryOpNode* node) {
    TABS
    int pprec = prec;
    prec = 1000000;
    stream << SHOW_OP(kind);
    node->operand()->visit(this);
    prec = pprec;
}

void ShowVisitor::visitStringLiteralNode(mathvm::StringLiteralNode* node) {
    TABS
    stream << '\'';
    const std::string& s = node->literal();
    for (const char *c = s.c_str(), *l = s.c_str(); 1; ++c) {
        switch (*c) {
            case '\a': stream.write(l, c - l); l = c + 1; stream << "\\a"; break;
            case '\b': stream.write(l, c - l); l = c + 1; stream << "\\b"; break;
            case '\t': stream.write(l, c - l); l = c + 1; stream << "\\t"; break;
            case '\n': stream.write(l, c - l); l = c + 1; stream << "\\n"; break;
            case '\f': stream.write(l, c - l); l = c + 1; stream << "\\f"; break;
            case '\r': stream.write(l, c - l); l = c + 1; stream << "\\r"; break;
            case '\'': stream.write(l, c - l); l = c + 1; stream << "\\'"; break;
            case '\\': stream.write(l, c - l); l = c + 1; stream << "\\\\"; break;
            case 0   : stream.write(l, c - l); goto end;
        }
    } end:
    stream << '\'';
}

void ShowVisitor::visitDoubleLiteralNode(mathvm::DoubleLiteralNode* node) {
    TABS
    std::stringstream str;
    str << node->literal();
    const std::string& s = str.str();
    size_t r = s.find('e');
    if (r != std::string::npos && r < s.length() - 1 && s[r + 1] == '+') {
        stream << s.substr(0, r + 1) << s.substr(r + 2);
    } else {
        stream << node->literal();
    }
}

void ShowVisitor::visitIntLiteralNode(mathvm::IntLiteralNode* node) {
    TABS
    stream << node->literal();
}

void ShowVisitor::visitLoadNode(mathvm::LoadNode* node) {
    TABS
    stream << node->var()->name();
}

void ShowVisitor::visitStoreNode(mathvm::StoreNode* node) {
    TABS
    stream << node->var()->name() << ' ' << SHOW_OP(op) << ' ';
    node->value()->visit(this);
    stream << ';';
}

void ShowVisitor::visitForNode(mathvm::ForNode* node) {
    TABS
    stream << "for (" << node->var()->name() << " in ";
    node->inExpr()->visit(this);
    ENTER(true, body)
}

void ShowVisitor::visitWhileNode(mathvm::WhileNode* node) {
    TABS
    stream << "while (";
    node->whileExpr()->visit(this);
    ENTER(true, loopBlock)
}

void ShowVisitor::visitIfNode(mathvm::IfNode* node) {
    TABS
    stream << "if (";
    node->ifExpr()->visit(this);
    ENTER(true, thenBlock)
    if (node->elseBlock()) {
        stream << " else ";
        ENTER(false, elseBlock)
    }
}

void ShowVisitor::visitBlockNode(mathvm::BlockNode* node) {
    mathvm::Scope::VarIterator it(node->scope());
    if (it.hasNext()) {
        while (it.hasNext()) {
            mathvm::AstVar* v = it.next();
            TABS
            need_tabs = true;
            stream << SHOW_TYPE(v->type()) << ' ' << v->name() << ";\n";
        }
        TABS
        need_tabs = true;
        stream << '\n';
    }
    for (uint32_t i = 0; i < node->nodes(); ++i) {
        node->nodeAt(i)->visit(this);
        stream << '\n';
        need_tabs = true;
    }
}

void ShowVisitor::visitFunctionNode(mathvm::FunctionNode* node) {
    TABS
    stream << "function " << node->name() << '(';
    node->args()->visit(this);
    ENTER(true, body)
}

void ShowVisitor::visitPrintNode(mathvm::PrintNode* node) {
    TABS
    stream << "print(";
    if (node->operands() > 0) {
        node->operandAt(0)->visit(this);
        for (uint32_t i = 1; i < node->operands(); ++i) {
            stream << ", ";
            node->operandAt(i)->visit(this);
        }
    }
    stream << ");";
}
