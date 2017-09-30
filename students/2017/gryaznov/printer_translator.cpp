#include "printer_translator.h"

#include <iostream>
#include <limits>
#include <sstream>

#include "mathvm.h"
#include "parser.h"

namespace mathvm
{

class PrettyPrinter final : public AstVisitor
{
public:
    PrettyPrinter(AstFunction* top)
        : _top(top)
        , _indent(0)
        , _status(nullptr)
    {}

    ~PrettyPrinter() override
    {}

    Status* print();

#define VISITOR_FUNCTION(type, name)            \
    void visit##type(type* node) override;

    FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION

private:
    void writeIndent();
    void write();
    void write(char c);
    void write(std::string const& str);
    void write(TokenKind const& kind);
    void write(VarType const& type);
    void indentInc();
    void indentDec();
    void error(const char* msg);

    AstFunction* _top;
    int _indent;
    Status* _status;

    static const int _indent_size;
};

Status* PrinterTranslator::translate(string const& program, Code** code)
{
    *code = nullptr;

    Parser parser;
    Status* parserStatus = parser.parseProgram(program);
    if (parserStatus->isError())
    {
        return parserStatus;
    }
    delete parserStatus;

    PrettyPrinter printer(parser.top());
    Status* printerStatus = printer.print();

    return printerStatus;
}

static bool isNativeFunction(FunctionNode* node)
{
    auto body = node->body();
    return body->nodes() != 0 && body->nodeAt(0)->isNativeCallNode();
}

const int PrettyPrinter::_indent_size = 4;

Status* PrettyPrinter::print()
{
    _top->node()->body()->visit(this);
    if (_status != nullptr)
    {
        return _status;
    }
    return Status::Ok();
}

void PrettyPrinter::writeIndent()
{
    std::cout << std::string(_indent * _indent_size, ' ');
}

void PrettyPrinter::write()
{
    std::cout << "\n";
}

void PrettyPrinter::write(char c)
{
    std::cout << c;
}

void PrettyPrinter::write(std::string const& str)
{
    std::cout << str;
}

void PrettyPrinter::write(TokenKind const& kind)
{
    switch(kind)
    {
#define ENUM_ELEM(t, s, p) \
    case t: \
        write(s); \
        break;
    FOR_TOKENS(ENUM_ELEM)
#undef ENUM_ELEM
    case tTokenCount:
        error("Can't print tTokenCount TokenKind");
        break;
    }
}

void PrettyPrinter::write(VarType const& type)
{
    switch (type)
    {
    case VT_INVALID:
        error("Invalid type");
        break;
    case VT_VOID:
        write("void");
        break;
    case VT_DOUBLE:
        write("double");
        break;
    case VT_INT:
        write("int");
        break;
    case VT_STRING:
        write("string");
        break;
    }
}

void PrettyPrinter::indentInc()
{
    ++_indent;
}

void PrettyPrinter::indentDec()
{
    --_indent;
}

void PrettyPrinter::error(const char* msg)
{
    if (_status == nullptr)
    {
        _status = Status::Error(msg);
    }
}

void PrettyPrinter::visitBinaryOpNode(BinaryOpNode* node)
{
    write("(");
    node->left()->visit(this);
    write(" ");
    write(node->kind());
    write(" ");
    node->right()->visit(this);
    write(")");
}

void PrettyPrinter::visitUnaryOpNode(UnaryOpNode* node)
{
    write("(");
    write(node->kind());
    node->operand()->visit(this);
    write(")");
}

void PrettyPrinter::visitStringLiteralNode(StringLiteralNode* node)
{
    write("'");
    for (auto c : node->literal())
    {
        switch (c)
        {
        case '\n':
            write("\\n");
            break;
        case '\r':
            write("\\r");
            break;
        case '\\':
            write("\\");
            break;
        case '\t':
            write("\\t");
            break;
        default:
            write(c);
        }
    }
    write("'");
}

void PrettyPrinter::visitDoubleLiteralNode(DoubleLiteralNode* node)
{
    std::stringstream ss;
    ss.precision(std::numeric_limits<double>::max_digits10);
    ss << node->literal();
    write(ss.str());
}

void PrettyPrinter::visitIntLiteralNode(IntLiteralNode* node)
{
    std::stringstream ss;
    ss << node->literal();
    write(ss.str());
}

void PrettyPrinter::visitLoadNode(LoadNode* node)
{
    write(node->var()->name());
}

void PrettyPrinter::visitStoreNode(StoreNode* node)
{
    write(node->var()->name());
    write(" ");
    write(node->op());
    write(" ");
    node->value()->visit(this);
}

void PrettyPrinter::visitForNode(ForNode* node)
{
    write("for (");
    write(node->var()->name());
    write(" in ");
    node->inExpr()->visit(this);
    write(") {");
    write();
    indentInc();
    node->body()->visit(this);
    indentDec();
    writeIndent();
    write("}");
}

void PrettyPrinter::visitWhileNode(WhileNode* node)
{
    write("while (");
    node->whileExpr()->visit(this);
    write(") {");
    write();
    indentInc();
    node->loopBlock()->visit(this);
    indentDec();
    writeIndent();
    write("}");
}

void PrettyPrinter::visitIfNode(IfNode* node)
{
    write("if (");
    node->ifExpr()->visit(this);
    write(") {");
    write();
    indentInc();
    node->thenBlock()->visit(this);
    indentDec();
    writeIndent();
    write("}");
    if (node->elseBlock())
    {
        write(" else {");
        write();
        indentInc();
        node->elseBlock()->visit(this);
        indentDec();
        writeIndent();
        write("}");
    }
}

void PrettyPrinter::visitBlockNode(BlockNode* node)
{
    for (auto it = Scope::VarIterator(node->scope()); it.hasNext();)
    {
        auto var = it.next();
        writeIndent();
        write(var->type());
        write(" ");
        write(var->name());
        write(";");
        write();
    }
    for (auto it = Scope::FunctionIterator(node->scope()); it.hasNext();)
    {
        auto fun = it.next();
        writeIndent();
        fun->node()->visit(this);
        write();
    }
    for (uint32_t i = 0; i < node->nodes(); ++i)
    {
        writeIndent();
        auto child = node->nodeAt(i);
        child->visit(this);
        if (!(child->isForNode() || child->isWhileNode() || child->isIfNode()))
        {
            write(";");
        }
        write();
    }
}

void PrettyPrinter::visitFunctionNode(FunctionNode* node)
{
    write("function ");
    write(node->returnType());
    write(" ");
    write(node->name());
    write("(");
    for (uint32_t i = 0; i < node->parametersNumber(); ++i)
    {
        if (i != 0)
        {
            write(", ");
        }
        write(node->parameterType(i));
        write(" ");
        write(node->parameterName(i));
    }
    write(") ");
    if (isNativeFunction(node))
    {
        node->body()->nodeAt(0)->visit(this);
        write(";");
    }
    else
    {
        write("{");
        write();
        indentInc();
        node->body()->visit(this);
        indentDec();
        writeIndent();
        write("}");
    }
}

void PrettyPrinter::visitReturnNode(ReturnNode* node)
{
    write("return");
    if (node->returnExpr())
    {
        write(" ");
        node->returnExpr()->visit(this);
    }
}

void PrettyPrinter::visitCallNode(CallNode* node)
{
    write(node->name());
    write("(");
    for (uint32_t i = 0; i < node->parametersNumber(); i++) {
        if (i != 0)
        {
            write(", ");
        }
        node->parameterAt(i)->visit(this);
    }
    write(")");
}

void PrettyPrinter::visitNativeCallNode(NativeCallNode* node)
{
    write("native '");
    write(node->nativeName());
    write("'");
}

void PrettyPrinter::visitPrintNode(PrintNode* node)
{
    write("print(");
    for (uint32_t i = 0; i < node->operands(); ++i) {
        if (i != 0)
        {
            write(", ");
        }
        node->operandAt(i)->visit(this);
    }
    write(")");
}

}
