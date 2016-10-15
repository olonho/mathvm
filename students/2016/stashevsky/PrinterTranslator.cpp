#include "PrinterTranslator.h"

#include "parser.h"

namespace mathvm {

using namespace std;

namespace details {

PrinterVisitor::~PrinterVisitor() {}

void PrinterVisitor::visitBinaryOpNode(BinaryOpNode* node) {
    node->left()->visit(this);
    cout << " " << tokenOp(node->kind()) << " ";
    node->right()->visit(this);
}

void PrinterVisitor::visitUnaryOpNode(UnaryOpNode* node) {
    cout << tokenOp(node->kind());
    format_ = false;
    node->visitChildren(this);
    format_ = true;
}

void PrinterVisitor::visitStringLiteralNode(StringLiteralNode* node) {
    cout << "'";
    for (auto c : node->literal()) {
        cout << escape(c);
    }
    cout << "'";
}

void PrinterVisitor::visitDoubleLiteralNode(DoubleLiteralNode* node) {
    cout << node->literal();
}

void PrinterVisitor::visitIntLiteralNode(IntLiteralNode* node) {
    cout << node->literal();
}

void PrinterVisitor::visitLoadNode(LoadNode* node) {
    cout << node->var()->name();
}

void PrinterVisitor::visitStoreNode(StoreNode* node) {
    printIndent();
    cout << node->var()->name() << " " << tokenOp(node->op()) << " ";
    format_ = false;
    node->value()->visit(this);
    format_ = true;

    printSeparator();
}

void PrinterVisitor::visitForNode(ForNode* node) {
    printIndent();

    cout << "for (" << node->var()->name() << " in ";
    format_ = false;
    node->inExpr()->visit(this);
    format_= true;
    cout << ")" << endl;

    node->body()->visit(this);
}

void PrinterVisitor::visitWhileNode(WhileNode* node) {
    printIndent();
    cout << "while (";
    node->whileExpr()->visit(this);
    cout << ")" << endl;
    node->loopBlock()->visit(this);
}

void PrinterVisitor::visitIfNode(IfNode* node) {
    printIndent();

    cout << "if (";
    node->ifExpr()->visit(this);
    cout << ")" << endl;

    node->thenBlock()->visit(this);

    if (node->elseBlock()) {
        printIndent();
        cout << "else" << endl;
        node->elseBlock()->visit(this);
    }
}

void PrinterVisitor::visitBlockNode(BlockNode* node) {
    if (level_ != 0) {
        printIndent();
        cout << "{" << endl;
    }
    ++level_;

    auto scope = node->scope();
    if (scope) {
        printScope(scope);
    }

    node->visitChildren(this);

    --level_;
    if (level_ != 0) {
        printIndent();
        cout << "}" << endl;
    }
}

void PrinterVisitor::visitFunctionNode(FunctionNode* node) {
    printIndent();
    cout << "function " << typeToName(node->returnType()) << " " << node->name() << "(";

    uint32_t parametersNumber = node->parametersNumber();
    for (uint32_t i = 0; i < parametersNumber; ++i) {
        cout << typeToName(node->parameterType(i)) << " " << node->parameterName(i);

        if (i != parametersNumber - 1) {
            cout << ", ";
        }
    }

    cout << ") ";

    if (node->body()->nodeAt(0)->isNativeCallNode()) {
        auto nativeCall = node->body()->nodeAt(0)->asNativeCallNode();
        cout << "native '" << nativeCall->nativeName() << "'";
        printSeparator();
        return;
    }

    cout << endl;
    node->visitChildren(this);
}

void PrinterVisitor::visitReturnNode(ReturnNode* node) {
    printIndent();

    cout << "return ";
    format_ = false;
    node->visitChildren(this);
    format_ = true;

    printSeparator();
}

void PrinterVisitor::visitCallNode(CallNode* node) {
    printIndent();

    cout << node->name() << "(";
    uint32_t parametersNumber = node->parametersNumber();

    for (uint32_t i = 0; i < parametersNumber; i++) {
        node->parameterAt(i)->visit(this);

        if (i != parametersNumber - 1) {
            cout << ", ";
        }
    }

    cout << ")";
    printSeparator();
}

void PrinterVisitor::visitNativeCallNode(NativeCallNode* node) {
    assert(false);
}

void PrinterVisitor::visitPrintNode(PrintNode* node) {
    printIndent();
    cout << "print(";

    uint32_t operands = node->operands();
    format_= false;
    for (uint32_t i = 0; i < operands; ++i) {
        node->operandAt(i)->visit(this);

        if (i != operands - 1) {
            cout << ", ";
        }

    }
    format_ = true;

    cout << ")";
    printSeparator();
}

void PrinterVisitor::printScope(Scope *scope) {

    auto var_iter = Scope::VarIterator(scope);
    while (var_iter.hasNext()) {
        auto variable = var_iter.next();
        printIndent();
        cout << typeToName(variable->type()) << " " << variable->name();
        printSeparator();
    }

    auto function_iter = Scope::FunctionIterator(scope);
    while (function_iter.hasNext()) {
        auto function = function_iter.next();
        function->node()->visit(this);
    }
}

void PrinterVisitor::printIndent() {
    if (!format_ || level_ < 2) {
        return;
    }

    cout << string((level_ - 1) * 2, ' ');
}

void PrinterVisitor::printSeparator() {
    if (!format_) {
        return;
    }

    cout << ";" << endl;
}

string PrinterVisitor::escape(char c) const {
    switch (c) {
        case '\'':
            return "\\'";
        case '\"':
            return "\\\"";
        case '\?':
            return "\\?";
        case '\\':
            return "\\\\";
        case '\a':
            return "\\a";
        case '\b':
            return "\\b";
        case '\f':
            return "\\f";
        case '\n':
            return "\\n";
        case '\r':
            return "\\r";
        case '\t':
            return "\\t";
        case '\v':
            return "\\v";
        default:
            return string(1, c);
    }
}

}

PrinterTranslator::~PrinterTranslator() {}

Status *PrinterTranslator::translate(const string &program, Code **code) {
    Parser parser;
    auto status = parser.parseProgram(program);
    if (status->isError()) {
        return status;
    }

    details::PrinterVisitor visitor;
    parser.top()->node()->visitChildren(&visitor);

    return Status::Ok();
}

}
