#include "mathvm.h"
#include "parser.h"
#include "source_translator.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>

using namespace std;
using namespace mathvm;

static const char* serializeToken(TokenKind kind)
{
    #define ENUM_ELEM(token, name, p) \
    case token: \
    { \
        return name; \
    }

    switch (kind) {
        FOR_TOKENS(ENUM_ELEM)
        default:
            break;
    }
    assert(false && "unexpected token");
    return NULL;
}

static const char* serializeType(VarType type)
{
    switch (type) {
        case VT_INVALID:
            return "invalid_type";
        case VT_VOID:
            return "void";
        case VT_DOUBLE:
            return "double";
        case VT_INT:
            return "int";
        case VT_STRING:
            return "string";
    }
    assert(false && "invalid type");
    return NULL;
}

namespace {

struct PrinterVisitor
    : public AstVisitor
{
    stringstream ss;
    int indent;
    bool bypass = false;
    bool block = false;

    PrinterVisitor()
        : indent(0)
    {}

    void printIndention() {
        static const int INDENTION_SPACES = 4;

        for (int i = 0; i < indent * INDENTION_SPACES; ++i)
            ss << ' ';
    }

    void visitBinaryOpNode(BinaryOpNode* node)
    {
        ss << "(";
        node->left()->visit(this);
        ss << ' ' << serializeToken(node->kind()) << ' ';
        node->right()->visit(this);
        ss << ")";
    }

    void visitUnaryOpNode(UnaryOpNode* node)
    {
        ss << serializeToken(node->kind());
        node->operand()->visit(this);
    }

    void visitStringLiteralNode(StringLiteralNode* node)
    {
        ss << '\'';
        for (char c : node->literal()) {
            switch (c) {
            case '\n':
                ss << "\\n";
                break;
            case '\r':
                ss << "\\r";
                break;
            case '\t':
                ss << "\\t";
                break;
            case '\\':
                ss << "\\\\";
                break;
            default:
                ss << c;
                break;
            }
        }
        ss << '\'';
    }

    void visitDoubleLiteralNode(DoubleLiteralNode* node)
    {
        ss << showpoint << node->literal();
    }

    void visitIntLiteralNode(IntLiteralNode* node)
    {
        ss << node->literal();
    }

    void visitLoadNode(LoadNode* node)
    {
        ss << node->var()->name();
    }

    void visitStoreNode(StoreNode* node)
    {
        printIndention();
        ss << node->var()->name() << ' ' << serializeToken(node->op()) << ' ';
        node->value()->visit(this);
    }

    void visitForNode(ForNode* node)
    {
        printIndention();
        ss << "for (" << node->var()->name() << " in ";
        node->inExpr()->visit(this);
        ss << ") ";
        node->body()->visit(this);
        ss << "\n";
    }

    void visitWhileNode(WhileNode* node)
    {
        printIndention();
        ss << "while (";
        node->whileExpr()->visit(this);
        ss << ") ";
        node->loopBlock()->visit(this);
        ss << "\n";
    }

    void visitIfNode(IfNode* node)
    {
        printIndention();
        ss << "if (";
        node->ifExpr()->visit(this);
        ss << ") ";
        node->thenBlock()->visit(this);

        if (node->elseBlock() != NULL) {
            ss << "\n";
            printIndention();
            ss << "else ";
            node->elseBlock()->visit(this);
        }

        ss << '\n';
    }

    void visitBlockNode(BlockNode* node)
    {
        if (node->nodes() > 1 && node->nodeAt(0)->isNativeCallNode()) {
            node->nodeAt(0)->visit(this);
            ss << ";";
            bypass = false;
            return;
        }

        bool bypassed = bypass;
        bypass = false;

        if (!bypassed) {
            ss << "{\n";
            ++indent;
        }

        Scope *scope = node->scope();

        // local variables
        Scope::VarIterator it(scope);
        while (it.hasNext()) {
            AstVar *var = it.next();
            printIndention();
            ss << serializeType(var->type()) << ' ' << var->name() << ";\n";
        }

        // functions
        Scope::FunctionIterator itf(scope);
        while (itf.hasNext()) {
            AstFunction *foo = itf.next();
            foo->node()->visit(this);
        }


        // statements
        for (uint32_t i = 0; i < node->nodes(); ++i) {
            AstNode *cld = node->nodeAt(i);
            block = false;
            if (cld->isNativeCallNode() || cld->isCallNode())
                printIndention();
            cld->visit(this);
            if (!block)
                ss << ";\n";
        }

        if (!bypassed) {
            --indent;
            printIndention();
            ss << "}";
        }
        block = true;
    }

    void visitFunctionNode(FunctionNode* node)
    {
        bypass = (node->name() == "<top>");

        if (!bypass) {
            printIndention();
            ss << "function " << serializeType(node->returnType())
               << ' ' << node->name() << '(';

            for (uint32_t i = 0; i < node->parametersNumber(); ++i) {
                ss << serializeType(node->parameterType(i)) << ' ' << node->parameterName(i);
                if (i < node->parametersNumber() - 1)
                    ss << ", ";
            }
            ss << ") ";
        }

        node->body()->visit(this);
        if (node->name() != "<top>")
            ss << "\n";
    }

    void visitReturnNode(ReturnNode* node)
    {
        printIndention();
        ss << "return";
        if (node->returnExpr()) {
            ss << ' ';
            node->visitChildren(this);
        }
    }

    void visitCallNode(CallNode* node)
    {
        ss << node->name() << '(';
        for (uint32_t i = 0; i < node->parametersNumber(); ++i) {
            node->parameterAt(i)->visit(this);
            if (i < node->parametersNumber() - 1)
                ss << ", ";
        }
        ss << ')';
    }

    void visitNativeCallNode(NativeCallNode* node)
    {
        printIndention();
        ss << "native '" << node->nativeName() << '\'';
    }

    void visitPrintNode(PrintNode* node)
    {
        printIndention();
        ss << "print(";
        for (uint32_t i = 0; i < node->operands(); ++i) {
            node->operandAt(i)->visit(this);
            if (i < node->operands() - 1)
                ss << ", ";
        }
        ss << ")";
    }
};


}


Status *SourceTranslatorImpl::translate(const string &program, Code **code)
{
    Parser parser;
    Status* status = parser.parseProgram(program);

    if (status->isError())
        return status;

    PrinterVisitor visitor;
    parser.top()->node()->visit(&visitor);
    cout << visitor.ss.str();

    return status;
}
