#include "pretty_print_translator_impl.hpp"

using namespace std;
using namespace mathvm;

template<typename Iterator, typename Consumer>
void iterate(Iterator& it, Consumer const& p) {
    while (it.hasNext()) {
        p(it.next());
    }
}

template<typename Printer>
void printWithSeparator(ostream& _buffer, size_t times, Printer const& p) {
    bool first = true;
    for (size_t i = 0; i < times; i++) {
        if (!first) {
            _buffer << ", ";
        }

        p(i);
        first = false;
    }
}

string escape(const string& s) {
    stringstream ss;
    for (char c : s) {
        switch (c) {
            case '\n':
                ss << "\\n";
                break;
            case '\t':
                ss << "\\t";
                break;
            case '\r':
                ss << "\\r";
                break;
            case '\\':
                ss << "\\\\";
                break;
            case '\'':
                ss << "\\'";
                break;
            default:
                ss << c;
        }
    }

    return ss.str();
}

ostream& operator<<(ostream& o, VarType type) {
    switch (type) {
        case VT_INVALID:
            o << "invalid";
            break;
        case VT_VOID:
            o << "void";
            break;
        case VT_DOUBLE:
            o << "double";
            break;
        case VT_INT:
            o << "int";
            break;
        case VT_STRING:
            o << "string";
            break;
    }

    return o;
}

ostream& operator<<(ostream& o, TokenKind kind) {
#define PRINT_OP(t, s, p) \
        case t: o << (s); \
        break;

    switch (kind) {
        case tTokenCount:
            break;
        FOR_TOKENS(PRINT_OP)
    }

#undef PRINT_OP

    return o;
}

void PrettyPrintTranslatorImpl::visitBlockNode(BlockNode* node) {
    Scope::VarIterator varIterator(node->scope());
    iterate(varIterator, [&](AstVar* var) {
        indent();
        _buffer << var->type()
                << " "
                << var->name()
                << ";"
                << endl;
    });

    Scope::FunctionIterator funIterator(node->scope());
    iterate(funIterator, [&](AstFunction* fun) {
        indent();
        fun->node()->visit(this);
        _buffer << endl;
    });

    for (size_t i = 0; i < node->nodes(); i++) {
        AstNode* currNode = node->nodeAt(i);
        if (currNode->isNativeCallNode()) continue;

        indent();

        currNode->visit(this);
        if (!(currNode->isForNode()
              || currNode->isWhileNode()
              || currNode->isIfNode()
              || currNode->isFunctionNode())) {
            _buffer << ";";
        }

        _buffer << endl;
    }
}

void PrettyPrintTranslatorImpl::visitFunctionNode(FunctionNode* node) {
    if (node->name() == AstFunction::top_name) {
        visitTopBlock(node->body());
        return;
    }

    _buffer << "function " << node->returnType() << " " << node->name() << "(";

    printWithSeparator(_buffer, node->parametersNumber(), [&](size_t i) {
        _buffer << node->parameterType(i) << " " << node->parameterName(i);
    });

    _buffer << ")";

    if (node->body()->nodeAt(0)->isNativeCallNode()) {
        _buffer << " ";
        node->body()->nodeAt(0)->visit(this);
        _buffer << ";";
    } else {
        _buffer << " {" << endl;

        increaseIndent();
        node->body()->visit(this);
        decreaseIndent();

        indent();
        _buffer << "}";
    }
}

void PrettyPrintTranslatorImpl::visitCallNode(CallNode* node) {
    _buffer << node->name() << "(";

    printWithSeparator(_buffer, node->parametersNumber(), [&](size_t i) {
        node->parameterAt(i)->visit(this);
    });

    cout << ")";
}

void PrettyPrintTranslatorImpl::visitPrintNode(PrintNode* node) {
    _buffer << "print(";

    printWithSeparator(_buffer, node->operands(), [&](size_t i) {
        node->operandAt(i)->visit(this);
    });

    cout << ")";
}

void PrettyPrintTranslatorImpl::visitStringLiteralNode(StringLiteralNode* node) {
    _buffer << "'" << escape(node->literal()) << "'";
}


void PrettyPrintTranslatorImpl::visitBinaryOpNode(BinaryOpNode* node) {
    _buffer << "(";
    node->left()->visit(this);
    _buffer << node->kind();
    node->right()->visit(this);
    _buffer << ")";
}

void PrettyPrintTranslatorImpl::visitUnaryOpNode(UnaryOpNode* node) {
    _buffer << "(";
    _buffer << node->kind();
    node->operand()->visit(this);
    _buffer << ")";
}

void PrettyPrintTranslatorImpl::visitNativeCallNode(NativeCallNode* node) {
    _buffer << " native " << "'" << node->nativeName() << "'";
}

void PrettyPrintTranslatorImpl::visitStoreNode(StoreNode* node) {
    _buffer << node->var()->name()
            << ' '
            << node->op()
            << ' ';
    node->value()->visit(this);
}

void PrettyPrintTranslatorImpl::visitDoubleLiteralNode(DoubleLiteralNode* node) {
    _buffer << node->literal();
}

void PrettyPrintTranslatorImpl::visitIntLiteralNode(IntLiteralNode* node) {
    _buffer << node->literal();
}

void PrettyPrintTranslatorImpl::visitLoadNode(LoadNode* node) {
    _buffer << node->var()->name();
}

Status* PrettyPrintTranslatorImpl::translate(const string& program, Code**) {
    mathvm::Parser parser;
    Status* parseStatus = parser.parseProgram(program);

    if (parseStatus->isOk()) {
        parser.top()->node()->visit(this);
    }

    return parseStatus;
}

void PrettyPrintTranslatorImpl::visitForNode(ForNode* node) {
    _buffer << "for (" << node->var()->name() << " in ";
    node->inExpr()->visit(this);
    _buffer << ") {" << endl;

    increaseIndent();
    node->body()->visit(this);
    decreaseIndent();

    indent();
    _buffer << "}" << endl;
}

void PrettyPrintTranslatorImpl::visitWhileNode(WhileNode* node) {
    _buffer << "while (";
    node->whileExpr()->visit(this);
    _buffer << ") {" << endl;

    increaseIndent();
    node->loopBlock()->visit(this);
    decreaseIndent();

    indent();
    _buffer << "}" << endl;
}

void PrettyPrintTranslatorImpl::visitIfNode(IfNode* node) {
    indent();
    _buffer << "if (";
    node->ifExpr()->visit(this);
    _buffer << ") {" << endl;

    increaseIndent();
    node->thenBlock()->visit(this);
    decreaseIndent();

    indent();
    _buffer << "}";
    if (node->elseBlock()) {
        _buffer << " else {" << endl;;

        increaseIndent();
        node->elseBlock()->visit(this);
        decreaseIndent();

        indent();
        _buffer << "}";
    }

}

void PrettyPrintTranslatorImpl::visitReturnNode(ReturnNode* node) {
    _buffer << "return";
    if (node->returnExpr()) {
        _buffer << " ";
        node->returnExpr()->visit(this);
    }
}

void PrettyPrintTranslatorImpl::visitTopBlock(BlockNode* node) {
    visitBlockNode(node);
}

void PrettyPrintTranslatorImpl::indent() {
    _buffer << string(_indent, ' ');
}

void PrettyPrintTranslatorImpl::increaseIndent() {
    _indent += _indentSize;
}

void PrettyPrintTranslatorImpl::decreaseIndent() {
    _indent -= _indentSize;
}

PrettyPrintTranslatorImpl::PrettyPrintTranslatorImpl(std::ostream& buffer, size_t indentSize) :
        _buffer(buffer), _indentSize(indentSize), _indent(0) {}
