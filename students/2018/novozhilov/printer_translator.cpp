//
// Created by jetbrains on 27.09.18.
//

#include <iomanip>
#include <typeinfo>
#include "parser.h"
#include "printer_translator.h"

using namespace mathvm;

Status *PrinterTranslator::translate(const std::string &program, Code **code __attribute__ ((unused))) {
    Parser parser;

    Status *status = parser.parseProgram(program);
    if (status->isOk()) {
        FunctionNode *topNode = parser.top()->node();
        PrettyPrinterVisitor printer(std::cout);
        printer.visitTopNode(topNode);
    }
    return status;
}

// --------------------------------- Visitor ---------------------------------

void PrettyPrinterVisitor::visitForNode(ForNode *node) {
    out << "for (" << node->var()->name() << " in ";
    node->inExpr()->visit(this);
    out << ") {" << endl;
    node->body()->visit(this);
    printIndent();
    out << "}";
}

void PrettyPrinterVisitor::visitPrintNode(PrintNode *node) {
    out << "print(";
    uint32_t operands = node->operands();
    if (operands > 0) {
        node->operandAt(0)->visit(this);
        for (uint32_t i = 1; i < operands; ++i) {
            out << ", ";
            node->operandAt(i)->visit(this);
        }
    }
    out << ")";
}

void PrettyPrinterVisitor::visitLoadNode(LoadNode *node) {
    out << node->var()->name();
}

void PrettyPrinterVisitor::visitIfNode(IfNode *node) {
    out << "if (";
    node->ifExpr()->visit(this);
    out << ") {" << endl;
    node->thenBlock()->visit(this);
    printIndent();
    out << "}";
    if (node->elseBlock()) {
        out << " else {" << endl;
        node->elseBlock()->visit(this);
        printIndent();
        out << "}";
    }
}

void PrettyPrinterVisitor::visitBinaryOpNode(BinaryOpNode *node) {
    node->left()->visit(this);
    printTokenKind(node->kind());
    node->right()->visit(this);
}

void PrettyPrinterVisitor::visitDoubleLiteralNode(DoubleLiteralNode *node) {
    out << node->literal();
}

void PrettyPrinterVisitor::visitStoreNode(StoreNode *node) {
    out << node->var()->name();
    printTokenKind(node->op());
    node->value()->visit(this);
}

std::string escapeString(std::string const &s) {
    stringstream ss;
    for (auto c : s) {
        switch (c) {
            case '\'': ss << "\\'";
                break;
            case '\n': ss << "\\n";
                break;
            case '\t': ss << "\\t";
                break;
            case '\\': ss << "\\\\";
                break;
            default: ss << c;
        }
    }
    return ss.str();
}

void PrettyPrinterVisitor::visitStringLiteralNode(StringLiteralNode *node) {
    out << "'" << escapeString(node->literal()) << "'";
}

void PrettyPrinterVisitor::visitWhileNode(WhileNode *node) {
    out << "while (";
    node->whileExpr()->visit(this);
    out << ") {" << endl;
    node->loopBlock()->visit(this);
    printIndent();
    out << "}" << endl;
}

void PrettyPrinterVisitor::visitIntLiteralNode(IntLiteralNode *node) {
    out << node->literal();
}

void PrettyPrinterVisitor::visitUnaryOpNode(UnaryOpNode *node) {
    printTokenKindNoSpaces(node->kind());
    node->operand()->visit(this);
}

void PrettyPrinterVisitor::visitNativeCallNode(NativeCallNode *node) {
    printIndent();
    out << "'" << node->nativeName() << "'";
}

void PrettyPrinterVisitor::visitBlockNode(BlockNode *node) {
    ++indent;

    Scope *scope = node->scope();

    Scope::VarIterator varIterator(scope);
    while (varIterator.hasNext()) {
        AstVar *variable = varIterator.next();
        printIndent();
        printVarType(variable->type());
        out << variable->name() << ";" << endl;
    }

    Scope::FunctionIterator functionIterator(scope);
    while (functionIterator.hasNext()) {
        AstFunction *function = functionIterator.next();
        function->node()->visit(this);
        out << endl;
    }

    uint32_t nodes = node->nodes();

    for (uint32_t i = 0; i < nodes; ++i) {
        printIndent();
        AstNode *childNode = node->nodeAt(i);
        childNode->visit(this);
        out << ";" << endl;
        if (childNode->isNativeCallNode()) {
            break;
        }
    }
    --indent;
}

void PrettyPrinterVisitor::visitTopNode(FunctionNode *node) {
    visitFunctionNode(node, true);
}

void PrettyPrinterVisitor::visitFunctionNode(FunctionNode *node) {
    visitFunctionNode(node, false);
}

bool isNativeFunc(FunctionNode *node) {
    if (node->body()->nodes() == 0) {
        return false;
    }
    return node->body()->nodeAt(0)->isNativeCallNode();
}

void PrettyPrinterVisitor::visitFunctionNode(FunctionNode *node, bool isTop) {
    bool isNativeFunction = isNativeFunc(node);

    if (!isTop) {
        printIndent();
        out << "function ";
        printVarType(node->returnType());
        out << node->name() << "(";
        uint32_t parametersNumber = node->parametersNumber();
        if (parametersNumber > 0) {
            printVarType(node->parameterType(0));
            out << " " << node->parameterName(0);
            for (uint32_t i = 1; i < parametersNumber; ++i) {
                out << ", ";
                printVarType(node->parameterType(i));
                out << " " << node->parameterName(i);
            }
        }
        out << ")";
        if (isNativeFunction) {
            out << " native ";
        } else {
            out << " {" << endl;
        }
    }

    node->body()->visit(this);

    if (!isTop && !isNativeFunction) {
        printIndent();
        out << "}" << endl;
    }
}

void PrettyPrinterVisitor::visitReturnNode(ReturnNode *node) {
    out << "return";
    if (node->returnExpr() != nullptr) {
        out << " ";
        node->returnExpr()->visit(this);
    }
}

void PrettyPrinterVisitor::visitCallNode(CallNode *node) {
    out << node->name() << "(";
    uint32_t parametersNumber = node->parametersNumber();
    if (parametersNumber > 0) {
        node->parameterAt(0)->visit(this);
        for (uint32_t i = 1; i < parametersNumber; ++i) {
            out << ", ";
            node->parameterAt(i)->visit(this);
        }
    }
    out << ")";
}

// -----------------------------------------------------

string PrettyPrinterVisitor::getTab() const {
    string result;
    result.reserve(TAB.length() * indent);
    for (int i = 0; i < indent; ++i) {
        result += TAB;
    }
    return result;
}

void PrettyPrinterVisitor::printIndent() const {
    out << getTab();
}

void PrettyPrinterVisitor::printVarType(VarType const &varType) const {
    switch (varType) {
        case VT_VOID: out << "void";
            break;
        case VT_DOUBLE: out << "double";
            break;
        case VT_INT: out << "int";
            break;
        case VT_STRING: out << "string";
            break;
        case VT_INVALID: out << "INVALID";
            break;
    }

    out << " ";
}

void PrettyPrinterVisitor::printTokenKind(TokenKind const &kind) const {
    out << " ";
    printTokenKindNoSpaces(kind);
    out << " ";
}

void PrettyPrinterVisitor::printTokenKindNoSpaces(TokenKind const &kind) const {
#define TOKEN_TO_STRING(t, s, p) if (kind == t) out << s;
    FOR_TOKENS(TOKEN_TO_STRING)
#undef TOKEN_TO_STRING
}
