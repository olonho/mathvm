#include "../my_include/astprinter_visitor.h"

using namespace mathvm;

void AstPrinterVisitor::visitBinaryOpNode(BinaryOpNode *node) {
    ss_ << '(';
    node->left()->visit(this);
    ss_ << ' ' << tokenOp(node->kind()) << ' ';
    node->right()->visit(this);
    ss_ << ')';
}

void AstPrinterVisitor::visitUnaryOpNode(UnaryOpNode *node) {
    ss_ << tokenOp(node->kind());
    node->visitChildren(this);
}

void AstPrinterVisitor::visitStringLiteralNode(StringLiteralNode *node) {
    ss_ << '\'' << escape(node->literal()) << '\'';
}

void AstPrinterVisitor::visitDoubleLiteralNode(DoubleLiteralNode *node) {
    ss_ << node->literal();
}

void AstPrinterVisitor::visitIntLiteralNode(IntLiteralNode *node) {
    ss_ << node->literal();
}

void AstPrinterVisitor::visitLoadNode(LoadNode *node) {
    ss_ << node->var()->name();
}

void AstPrinterVisitor::visitStoreNode(StoreNode *node) {
    indent(node->var()->name());
    ss_ << ' ' << tokenOp(node->op()) << ' ';
    node->value()->visit(this);
    ss_ << ";";
    newline();
}

void AstPrinterVisitor::visitForNode(ForNode *node) {
    indent("for ("); ss_ << node->var()->name() << " in "; node->inExpr()->visit(this); ss_ << ") {";
    newline();
    tab();
    node->body()->visit(this);
    untab();
    indent("}");
    newline();
}

void AstPrinterVisitor::visitWhileNode(WhileNode *node) {
    indent("while ("); node->whileExpr()->visit(this); ss_ << ") {";
    newline();
    tab();
    node->loopBlock()->visit(this);
    untab();
    indent("}");
    newline();
}

void AstPrinterVisitor::visitIfNode(IfNode *node) {

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

void AstPrinterVisitor::visitBlockNode(BlockNode *node) {

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

void AstPrinterVisitor::visitFunctionNode(FunctionNode *node) {
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

void AstPrinterVisitor::visitReturnNode(ReturnNode *node) {
    indent("return");
    if (node->returnExpr()) {
        ss_ << ' ';
        node->returnExpr()->visit(this);
    }
    ss_ << ";";
    newline();
}

void AstPrinterVisitor::visitCallNode(CallNode *node) {
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

void AstPrinterVisitor::visitNativeCallNode(NativeCallNode *node) {
    ss_ << "native '" << node->nativeName()<< "';";
    newline();
}

void AstPrinterVisitor::visitPrintNode(PrintNode *node) {
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

const string AstPrinterVisitor::get_program() const {
    return ss_.str();
}

void AstPrinterVisitor::newline() {
    ss_ << '\n';
    need_indent_ = true;
}

void AstPrinterVisitor::tab() {
    indent_ += 4;
}

void AstPrinterVisitor::untab() {
    indent_ -= 4;
}

string AstPrinterVisitor::escape_char(char c) {
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

string AstPrinterVisitor::escape(const string &str) {
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

string AstPrinterVisitor::type_str(VarType t) {
    vector<string> types = {"<invalid>", "void", "double", "int", "string"};
    return types[(int) t];
}

NativeCallNode *AstPrinterVisitor::check_native(FunctionNode *node) {
    BlockNode * block = node->body();
    AstNode * first_child = block->nodeAt(0);
    NativeCallNode * native = dynamic_cast<NativeCallNode*>(first_child);
    return native;
}
