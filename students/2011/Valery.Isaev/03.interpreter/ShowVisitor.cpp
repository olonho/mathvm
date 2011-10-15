#include <iostream>
#include <sstream>

#include "ShowVisitor.h"

const int tab = 4;

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
    TABS \
    stream << "}"; \
}

ShowVisitor::ShowVisitor(std::ostream& o): need_tabs(false),
    prec(0), level(0), stream(o) {}

void ShowVisitor::visitBinaryOpNode(mathvm::BinaryOpNode* node) {
    TABS
    int pprec = prec;
    prec = tokenPrecedence(node->kind());
    if (pprec > prec) stream << '(';
    node->left()->visit(this);
    stream << " " << tokenOp(node->kind()) << ' ';
    ++prec;
    node->right()->visit(this);
    if (pprec >= prec) stream << ')';
    prec = pprec;
}

void ShowVisitor::visitUnaryOpNode(mathvm::UnaryOpNode* node) {
    TABS
    int pprec = prec;
    prec = 1000000;
    stream << tokenOp(node->kind());
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
        if (r == std::string::npos && s.find('.') == std::string::npos) {
            stream << ".0";
        }
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
    stream << node->var()->name() << ' ' << tokenOp(node->op()) << ' ';
    node->value()->visit(this);
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
    mathvm::Scope::FunctionIterator it1(node->scope());
    if (it.hasNext()) {
        while (it.hasNext()) {
            mathvm::AstVar* v = it.next();
            TABS
            need_tabs = true;
            stream << typeToName(v->type()) << ' ' << v->name() << ";\n";
        }
        if (node->nodes() || it1.hasNext()) {
            TABS
            need_tabs = true;
            stream << '\n';
        }
    }
    if (it1.hasNext()) {
        while (it1.hasNext()) {
            mathvm::AstFunction* f = it1.next();
            TABS
            f->node()->visit(this);
            stream << '\n';
            need_tabs = true;
        }
        if (node->nodes()) {
            TABS
            need_tabs = true;
            stream << '\n';
        }
    }
    for (uint32_t i = 0; i < node->nodes(); ++i) {
        node->nodeAt(i)->visit(this);
        stream << ";\n";
        need_tabs = true;
    }
}

void ShowVisitor::visitFunctionNode(mathvm::FunctionNode* node) {
    TABS
    stream << "function " << typeToName(node->returnType())
        << ' ' << node->name() << '(';
    if (node->parametersNumber()) {
        stream << typeToName(node->parameterType(0))
                 << ' ' << node->parameterName(0);
        for (uint32_t i = 1; i < node->parametersNumber(); ++i) {
            stream << ", " << typeToName(node->parameterType(i))
                    << ' ' << node->parameterName(i);
        }
    }
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
    stream << ")";
}

void ShowVisitor::visitReturnNode(mathvm::ReturnNode* node) {
    TABS
    if (node->returnExpr()) {
        stream << "return ";
        node->returnExpr()->visit(this);
    } else {
        stream << "return";
    }
}

void ShowVisitor::visitCallNode(mathvm::CallNode* node) {
    TABS
    stream << node->name() << '(';
    if (node->parametersNumber()) {
        node->parameterAt(0)->visit(this);
        for (uint32_t i = 1; i < node->parametersNumber(); ++i) {
            stream << ", ";
            node->parameterAt(i)->visit(this);
        }
    }
    stream << ')';
}
