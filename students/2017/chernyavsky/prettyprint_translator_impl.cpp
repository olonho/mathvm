#include <iomanip>
#include <parser.h>
#include <sstream>
#include "prettyprint_translator.h"

namespace mathvm {

    static inline const char* typeToString(VarType type) {
        switch (type) {
            case VT_VOID:
                return "void";
            case VT_DOUBLE:
                return "double";
            case VT_INT:
                return "int";
            case VT_STRING:
                return "string";
            case VT_INVALID:
            default:
                return "<unknown>";
        }
    }

    static inline string escapeString(const string& str) {
        stringstream escapedStr;
        for (auto ch : str) {
            if (ch == '\'') {
                escapedStr << "\\'";
            } else if (ch == '\\') {
                escapedStr << "\\\\";
            } else if (ch == '\n') {
                escapedStr << "\\n";
            } else if (ch == '\r') {
                escapedStr << "\\r";
            } else if (ch == '\t') {
                escapedStr << "\\t";
            } else {
                escapedStr << ch;
            }
        }
        return escapedStr.str();
    }

    void PrettyPrintVisitor::visitTopNode(FunctionNode* node) {
        printStatements(node->body(), false);
        printNewLine();
    }

    void PrettyPrintVisitor::visitBinaryOpNode(BinaryOpNode* node) {
        _out << '(';
        node->left()->visit(this);
        _out << ' ' << tokenOp(node->kind()) << ' ';
        node->right()->visit(this);
        _out << ')';
    }

    void PrettyPrintVisitor::visitUnaryOpNode(UnaryOpNode* node) {
        _out << tokenOp(node->kind());
        node->operand()->visit(this);
    }

    void PrettyPrintVisitor::visitStringLiteralNode(StringLiteralNode* node) {
        _out << '\'' << escapeString(node->literal()) << '\'';
    }

    void PrettyPrintVisitor::visitDoubleLiteralNode(DoubleLiteralNode* node) {
        _out << setprecision(6) << fixed << node->literal();
    }

    void PrettyPrintVisitor::visitIntLiteralNode(IntLiteralNode* node) {
        _out << node->literal();
    }

    void PrettyPrintVisitor::visitLoadNode(LoadNode* node) {
        _out << node->var()->name();
    }

    void PrettyPrintVisitor::visitStoreNode(StoreNode* node) {
        _out << node->var()->name();
        _out << ' ' << tokenOp(node->op()) << ' ';
        node->value()->visit(this);
    }

    void PrettyPrintVisitor::visitForNode(ForNode* node) {
        _out << "for (";
        _out << node->var()->name() << " in ";
        node->inExpr()->visit(this);
        _out << ") ";
        node->body()->visit(this);
    }

    void PrettyPrintVisitor::visitWhileNode(WhileNode* node) {
        _out << "while (";
        node->whileExpr()->visit(this);
        _out << ") ";
        node->loopBlock()->visit(this);
    }

    void PrettyPrintVisitor::visitIfNode(IfNode* node) {
        _out << "if (";
        node->ifExpr()->visit(this);
        _out << ") ";
        node->thenBlock()->visit(this);
        if (node->elseBlock() != nullptr) {
            printNewLine();
            _out << "else ";
            node->elseBlock()->visit(this);
        }
    }

    void PrettyPrintVisitor::visitBlockNode(BlockNode* node) {
        _out << '{';
        indent();
        printStatements(node);
        dedent();
        printNewLine();
        _out << '}';
    }

    void PrettyPrintVisitor::visitFunctionNode(FunctionNode* node) {
        _out << "function " << typeToString(node->returnType()) << ' ' << node->name() << '(';
        if (node->parametersNumber() > 0) {
            for (uint32_t i = 0; i < node->parametersNumber(); i++) {
                if (i > 0) {
                    _out << ", ";
                }
                _out << typeToString(node->parameterType(i)) << ' ' << node->parameterName(i);
            }
        }
        _out << ") ";

        BlockNode* body = node->body();
        if (body->nodes() > 0 && body->nodeAt(0)->isNativeCallNode()) {
            body->nodeAt(0)->visit(this);
        } else {
            body->visit(this);
        }
    }

    void PrettyPrintVisitor::visitReturnNode(ReturnNode* node) {
        _out << "return";
        if (node->returnExpr() != nullptr) {
            _out << ' ';
            node->returnExpr()->visit(this);
        }
    }

    void PrettyPrintVisitor::visitCallNode(CallNode* node) {
        _out << node->name() << "(";
        if (node->parametersNumber() > 0) {
            for (uint32_t i = 0; i < node->parametersNumber(); i++) {
                if (i > 0) {
                    _out << ", ";
                }
                node->parameterAt(i)->visit(this);
            }
        }
        _out << ")";
    }

    void PrettyPrintVisitor::visitNativeCallNode(NativeCallNode* node) {
        _out << "native '" << node->nativeName() << "';";
    }

    void PrettyPrintVisitor::visitPrintNode(PrintNode* node) {
        _out << "print(";
        if (node->operands() > 0) {
            for (uint32_t i = 0; i < node->operands(); i++) {
                if (i > 0) {
                    _out << ", ";
                }
                node->operandAt(i)->visit(this);
            }
        }
        _out << ")";
    }

    void PrettyPrintVisitor::indent() {
        _offset += _indentation;
    }

    void PrettyPrintVisitor::dedent() {
        _offset -= _indentation;
    }

    void PrettyPrintVisitor::printNewLine() {
        _out << endl << string(_offset, ' ');
    }

    void PrettyPrintVisitor::printStatements(const BlockNode* node, bool insertNewLine) {
        Scope::VarIterator varIterator(node->scope());
        while (varIterator.hasNext()) {
            if (insertNewLine) {
                printNewLine();
            } else {
                insertNewLine = true;
            }

            AstVar* var = varIterator.next();
            _out << typeToString(var->type()) << ' ' << var->name() << ';';

        }

        Scope::FunctionIterator functionIterator(node->scope());
        while (functionIterator.hasNext()) {
            if (insertNewLine) {
                printNewLine();
            } else {
                insertNewLine = true;
            }

            AstFunction* function = functionIterator.next();
            function->node()->visit(this);
        }

        for (uint32_t i = 0; i < node->nodes(); i++) {
            if (insertNewLine) {
                printNewLine();
            } else {
                insertNewLine = true;
            }

            AstNode* lastNode = node->nodeAt(i);
            lastNode->visit(this);
            if (!lastNode->isForNode() && !lastNode->isWhileNode() && !lastNode->isIfNode()) {
                _out << ';';
            }
        }
    }

    Status* PrettyPrintTranslatorImpl::translate(const string& program, Code** result) {
        Parser parser;

        Status* status = parser.parseProgram(program);
        if (status->isOk()) {
            PrettyPrintVisitor prettyPrinter(cout);
            prettyPrinter.visitTopNode(parser.top()->node());
        }

        return status;
    }

}
