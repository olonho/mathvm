#include <iostream>
#include <unordered_map>
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

PrinterVisitor::PrinterVisitor(ostream &strm) : _expr_counter(0), _indent(0), _strm(strm) {

}

void PrinterVisitor::visitBinaryOpNode(BinaryOpNode *node) {
    node->left()->visit(this);
    _strm << ' ' << tokenOp(node->kind()) << ' ';
    node->right()->visit(this);
}

void PrinterVisitor::visitBlockNode(BlockNode *node) {
    static int counter = 0;

    if (counter != 0) {
        _strm << "{" << std::endl;
        _indent++;
    }
    counter++;

    for (Scope::VarIterator it(node->scope()); it.hasNext();) {
        AstVar *var = it.next();
        _strm << std::string(_indent * _indent_size, ' ');
        _strm << typeToName(var->type()) << " " << var->name() << ";" << std::endl;
    }

    for (Scope::FunctionIterator it(node->scope()); it.hasNext();) {
        AstFunction *foo = it.next();
        _strm << std::string(_indent * _indent_size, ' ');
        foo->node()->visit(this);
    }


    for (uint32_t i = 0; i < node->nodes(); ++i) {
        AstNode *cld = node->nodeAt(i);
        cld->visit(this);
    }


    counter--;
    if (counter != 0) {
        _indent--;
        _strm << std::string(_indent * _indent_size, ' ') << "}" << std::endl;
    }
}

void PrinterVisitor::visitCallNode(CallNode *node) {
    if (_expr_counter == 0) {
        _strm << std::string(_indent * _indent_size, ' ');
    }
    _strm << node->name() << '(';
    for (uint32_t i = 0; i < node->parametersNumber(); ++i) {
        _expr_counter++;
        node->parameterAt(i)->visit(this);
        _expr_counter--;
        if (i + 1 < node->parametersNumber()) {
            _strm << ", ";
        }
    }
    _strm << ")";
    if (_expr_counter == 0) {
        _strm << ";" << std::endl;
    }
}

void PrinterVisitor::visitDoubleLiteralNode(DoubleLiteralNode *node) {
    _strm << node->literal();
}

void PrinterVisitor::visitForNode(ForNode *node) {
    _strm << std::string(_indent * _indent_size, ' ');
    _strm << "for (" << node->var()->name() << " in ";
    _expr_counter++;
    node->inExpr()->visit(this);
    _expr_counter--;
    _strm << ")";

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
    _strm << std::string(_indent * _indent_size, ' ');
    _strm << "if (";
    _expr_counter++;
    node->ifExpr()->visit(this);
    _expr_counter--;
    _strm << ") ";

    node->thenBlock()->visit(this);

    if (node->elseBlock() != 0) {
        _strm << std::string(_indent * _indent_size, ' ');
        _strm << " else ";
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
    _strm << "native '" << node->nativeName() << '\'';
}

void PrinterVisitor::visitPrintNode(PrintNode *node) {
    _strm << "print(";
    for (uint32_t i = 0; i < node->operands(); i++) {
        _expr_counter++;
        node->operandAt(i)->visit(this);
        _expr_counter--;
        if (i + 1 != node->operands()) {
            _strm << ", ";
        }
    }
    _strm << ");" << std::endl;
}

void PrinterVisitor::visitReturnNode(ReturnNode *node) {
    _expr_counter++;
    _strm << std::string(_indent * _indent_size, ' ');
    _strm << "return ";
    node->visitChildren(this);
    _strm << ";" << std::endl;
    _expr_counter--;
}

void PrinterVisitor::visitStoreNode(StoreNode *node) {
    _expr_counter++;
    _strm << std::string(_indent * _indent_size, ' ');
    _strm << node->var()->name() << " " << tokenOp(node->op()) << " ";
    node->visitChildren(this);
    _strm << ";" << std::endl;
    _expr_counter--;
}

void PrinterVisitor::visitStringLiteralNode(StringLiteralNode *node) {
    static std::unordered_map<char, std::string> mapping = {
        { '\n', "\\n" }, { '\r', "\\r" }, { '\t', "\\t" },
        { '\'', "\\" }, { '\\', "\\\\" }
    };

    _strm << '\'';
    for (auto c : node->literal()) {
        auto it = mapping.find(c);
        if (it != mapping.end()) {
            _strm << it->second;
            continue;
        }
        _strm << c;
    }
    _strm << '\'';
}

void PrinterVisitor::visitUnaryOpNode(UnaryOpNode *node) {
    _strm << tokenOp(node->kind());
    node->visitChildren(this);
}

void PrinterVisitor::visitWhileNode(WhileNode *node) {
    _strm << std::string(_indent * _indent_size, ' ');
    _strm << "while (";
     _expr_counter++;
    node->whileExpr()->visit(this);
    _expr_counter--;
    _strm << std::endl;
    node->loopBlock()->visit(this);
}

}
