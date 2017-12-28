#include <iostream>
#include <unordered_map>
#include <iomanip>
#include "printer_translator_impl.h"
#include "ast.h"

namespace mathvm {

Status* PrinterTranslatorImpl::translate(const string& program, Code* *code) {
    Parser parser;
    Status* status = parser.parseProgram(program);

    if (status->isError()) {
        return status;
    }

    PrinterVisitor visitor(std::cout);
    parser.top()->node()->visit(&visitor);

    return status;
}

PrinterVisitor::PrinterVisitor(ostream &strm) : _indent(0), _strm(strm) {

}

void PrinterVisitor::visitBinaryOpNode(BinaryOpNode *node) {
    node->left()->visit(this);
    _strm << ' ' << tokenOp(node->kind()) << ' ';
    node->right()->visit(this);
}

void PrinterVisitor::indent() {
    _strm << std::string(_indent * _indent_size, ' ');
}

void PrinterVisitor::visitBlockNode(BlockNode *node) {
    static int counter = 0;

    if (counter != 0 && !(node->nodes() > 0 && node->nodeAt(0)->isNativeCallNode())) {
        _strm << "{" << std::endl;
        _indent++;
    }
    counter++;

    for (Scope::VarIterator it(node->scope()); it.hasNext();) {
        indent();
        AstVar *var = it.next();
        _strm << typeToName(var->type()) << " " << var->name() << ";" << std::endl;
    }

    for (Scope::FunctionIterator it(node->scope()); it.hasNext();) {
        indent();
        AstFunction *foo = it.next();
        foo->node()->visit(this);
    }

    for (uint32_t i = 0; i < node->nodes(); ++i) {
        indent();
        AstNode *cld = node->nodeAt(i);
        cld->visit(this);

        if (!cld->isIfNode() && !cld->isWhileNode() && !cld->isForNode())
            _strm << ';' << std::endl;

        if (cld->isNativeCallNode()) {
            break;
        }
    }

    counter--;
    if (counter != 0 && !(node->nodes() > 0 && node->nodeAt(0)->isNativeCallNode())) {
        _indent--;
        indent();
        _strm << "}" << std::endl;
    }
}

void PrinterVisitor::visitCallNode(CallNode *node) {
    _strm << node->name() << '(';
    for (uint32_t i = 0; i < node->parametersNumber(); ++i) {
        node->parameterAt(i)->visit(this);
        if (i + 1 < node->parametersNumber()) {
            _strm << ", ";
        }
    }
    _strm << ")";
}

void PrinterVisitor::visitDoubleLiteralNode(DoubleLiteralNode *node) {
    _strm << std::showpoint << node->literal();
}

void PrinterVisitor::visitForNode(ForNode *node) {
    _strm << "for (" << node->var()->name() << " in ";
    node->inExpr()->visit(this);
    _strm << ") ";

    node->body()->visit(this);
}

void PrinterVisitor::visitFunctionNode(FunctionNode *node) {
    if (node->name() != AstFunction::top_name) {
        _strm << "function " << typeToName(node->returnType()) << " " << node->name() << "(";

        for (uint32_t i = 0; i < node->parametersNumber(); ++i) {
            _strm << typeToName(node->parameterType(i)) << " " << node->parameterName(i);

            if (i + 1 != node->parametersNumber()) {
                _strm << ", ";
            }
        }
        _strm << ") ";
    }

    node->body()->visit(this);
}

void PrinterVisitor::visitIfNode(IfNode *node) {
    _strm << "if (";
    node->ifExpr()->visit(this);
    _strm << ") ";

    node->thenBlock()->visit(this);

    if (node->elseBlock() != 0) {
        indent();
        _strm << "else ";
        node->elseBlock()->visit(this);
    }
}

void PrinterVisitor::visitIntLiteralNode(IntLiteralNode *node) {
    _strm << node->literal();
}

void PrinterVisitor::visitLoadNode(LoadNode *node) {
    _strm << node->var()->name();
}

void PrinterVisitor::visitNativeCallNode(NativeCallNode *node) {
    _strm << "native '" << node->nativeName();
}

void PrinterVisitor::visitPrintNode(PrintNode *node) {
    _strm << "print(";
    for (uint32_t i = 0; i < node->operands(); i++) {
        node->operandAt(i)->visit(this);
        if (i + 1 != node->operands()) {
            _strm << ", ";
        }
    }
    _strm << ")";
}

void PrinterVisitor::visitReturnNode(ReturnNode *node) {
    _strm << "return ";
    node->visitChildren(this);
}

void PrinterVisitor::visitStoreNode(StoreNode *node) {
    _strm << node->var()->name() << " " << tokenOp(node->op()) << " ";
    node->visitChildren(this);
}

void PrinterVisitor::visitStringLiteralNode(StringLiteralNode *node) {
    static std::unordered_map<char, std::string> mapping = {
        { '\n', "\\n" }, { '\r', "\\r" }, { '\t', "\\t" },
        { '\'', "\\" }, { '\\', "\\\\" }
    };

    _strm << '\'';
    for (auto c : node->literal()) {
        const auto it = mapping.find(c);
        if (it != mapping.cend()) {
            _strm << it->second;
            continue;
        }
        _strm << c;
    }
    _strm << '\'';
}

void PrinterVisitor::visitUnaryOpNode(UnaryOpNode *node) {
    _strm << tokenOp(node->kind()) << "(";
    node->visitChildren(this);
    _strm << ")";
}

void PrinterVisitor::visitWhileNode(WhileNode *node) {
    _strm << "while (";
    node->whileExpr()->visit(this);
    _strm << ") ";
    node->loopBlock()->visit(this);
}

}
