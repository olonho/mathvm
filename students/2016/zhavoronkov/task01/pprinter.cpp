#include "pprinter.h"
#include "parser.h"

#include <iostream>
#include <string>
#include <cstdint>

namespace mathvm {

using std :: ostream;

namespace details {

PrettyPrinter :: PrettyPrinter(ostream& out, uint32_t indent)
    : _out(out)
    , _indent(indent) {  }

PrettyPrinter :: ~PrettyPrinter() {}

void PrettyPrinter :: visitBinaryOpNode(BinaryOpNode* node) {
    node->left()->visit(this);
    _out << " "  << tokenOp(node->kind()) << " ";
    node->right()->visit(this);
}

void PrettyPrinter :: visitUnaryOpNode(UnaryOpNode* node) {
    _out << tokenOp(node->kind());
    node->operand()->visit(this);
}

void PrettyPrinter :: visitStringLiteralNode(StringLiteralNode* node) {
    _out << "'";
    for (char c : node->literal()) {
        _out << escapeChar(c);
    }
    _out << "'";
}

void PrettyPrinter :: visitIntLiteralNode(IntLiteralNode* node) {
    _out << node->literal();
}

void PrettyPrinter :: visitDoubleLiteralNode(DoubleLiteralNode* node) {
    _out << node->literal();
}

void PrettyPrinter :: visitLoadNode(LoadNode* node) {
    indent();
    _out << node->var()->name();
}

void PrettyPrinter :: visitStoreNode(StoreNode* node) {
    indent();
    _out << node->var()->name() << " " << tokenOp(node->op()) << " ";
    _format = false;
    node->value()->visit(this);
    _format = true;
    separator();
}

void PrettyPrinter :: visitBlockNode(BlockNode* node) {
    indent();
    if (_indent != 0) {
        _out << "{\n";
    }
    ++_indent;

    Scope* scope = node->scope();
    if (scope) {
        printScope(scope);
    }

    node->visitChildren(this);

    --_indent;
    indent();
    if (_indent != 0) {
        _out << "}\n";
    }
}

void PrettyPrinter :: visitNativeCallNode(NativeCallNode* node) {
    _out << "native '" << node->nativeName() << "'";
    separator();
}

void PrettyPrinter :: visitForNode(ForNode* node) {
    indent();
    _out << "for (" << node->var()->name() << " in ";
    _format = false;
    node->inExpr()->visit(this);
    _format = true;
    _out << ")\n";
    node->body()->visit(this);
}

void PrettyPrinter :: visitWhileNode(WhileNode* node) {
    indent();
    _out << "while (";
    _format = false;
    node->whileExpr()->visit(this);
    _format = true;
    _out << ")\n";
    node->loopBlock()->visit(this);
}

void PrettyPrinter :: visitIfNode(IfNode* node) {
    indent();
    _out << "if (";
    _format = false;
    node->ifExpr()->visit(this);
    _format = true;
    _out << ")\n";

    node->thenBlock()->visit(this);

    if (node->elseBlock()) {
        indent();
        _out << "else\n";
        node->elseBlock()->visit(this);
    }
}

void PrettyPrinter :: visitReturnNode(ReturnNode* node) {
    indent();
    _out << "return ";
    if (node->returnExpr()) {
        _format = false;
        node->returnExpr()->visit(this);
        _format = true;
    }
    separator();
}

void PrettyPrinter :: visitFunctionNode(FunctionNode* node) {
    indent();

    _out << "function " << typeToName(node->returnType()) << " " << node->name() << "(";
    for (uint32_t i = 0; i < node->parametersNumber(); ++i) {
        _out << typeToName(node->parameterType(i)) << " " << node->parameterName(i);
        if (i != node->parametersNumber() - 1) {
            _out << ", ";
        }
    }
    _out << ") ";

    if (node->body()->nodeAt(0)->isNativeCallNode()) {
        node->body()->nodeAt(0)->asNativeCallNode()->visit(this);
    }

    _out << "\n";

    node->visitChildren(this);
}

void PrettyPrinter :: visitCallNode(CallNode* node) {
    indent();
    _out << node->name() << "(";
    for (uint32_t i = 0; i < node->parametersNumber(); ++i) {
        node->parameterAt(i)->visit(this);
        if (i != node->parametersNumber() - 1) {
            _out << ", ";
        }
    }
    _out << ")";
    separator();
}

void PrettyPrinter :: visitPrintNode(PrintNode* node) {
    indent();
    _out << "print" << "(";
    _format = false;
    for (uint32_t i = 0; i < node->operands(); ++i) {
        node->operandAt(i)->visit(this);
        if (i != node->operands() - 1) {
            _out << ", ";
        }
    }
    _format = true;
    _out << ")";
    separator();
}

string PrettyPrinter :: escapeChar(char c) {
    switch (c) {
        case '\'' :
            return "\\'";
        case '\"' :
            return "\\\"";
        case '\?' :
            return "\\\?";
        case '\\' :
            return "\\\\";
        case '\0' :
            return "\\0";
        case '\a' :
            return "\\a";
        case '\b' :
            return "\\b";
        case '\f' :
            return "\\f";
        case '\n' :
            return "\\n";
        case '\r' :
            return "\\r";
        case '\t' :
            return "\\t";
        case '\v' :
            return "\\v";
        default :
            return string(1, c);
    }
}

void PrettyPrinter :: printScope(Scope* scope) {
    Scope :: VarIterator varIterator = Scope :: VarIterator(scope);
    while (varIterator.hasNext()) {
        AstVar* var = varIterator.next();
        indent();
        _out << typeToName(var->type()) << " " << var->name();
        separator();
    }

    Scope :: FunctionIterator funcIterator = Scope :: FunctionIterator(scope);
    while(funcIterator.hasNext()) {
        AstFunction* function = funcIterator.next();
        function->node()->visit(this);
    }
}

void PrettyPrinter :: indent() {
    if (_format && _indent >= 2) {
        for (uint32_t i = 0; i < _indent; ++i) {
            _out << "\t";
        }
    }
}

void PrettyPrinter :: separator() {
    if (!_format) {
        return;
    }
    _out << ";\n";
}

} //end namespace details

PrinterTranslatorImpl :: PrinterTranslatorImpl(std :: ostream& out)
    : _pprinter(out, 0) { }

PrinterTranslatorImpl :: ~PrinterTranslatorImpl() {}

Status* PrinterTranslatorImpl :: translate(const string& program, Code** code) {
    Parser parser;

    Status* status = parser.parseProgram(program);
    if (status->isError()) {
        return status;
    }

    parser.top()->node()->visitChildren(&_pprinter);
    return Status::Ok();
}

} //end namespace mathvm
