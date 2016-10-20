#include "ast_printer.h"

using namespace mathvm;

AstPrinter::AstPrinter()
    : _indent_count(0)
{}

AstPrinter::~AstPrinter()
{}

std::string AstPrinter::getResult() const
{
    return _output.str();
}

std::string AstPrinter::escapeChar(char symbol)
{
    string result;
    switch (symbol)
    {
        case '\n':
            result = "\\n";
            break;
        case '\r':
            result = "\\r";
            break;
        case '\t':
            result = "\\t";
            break;
        case '\\':
            result = "\\\\";
            break;
        default:
            result = std::string(1, symbol);
            break;
    }

    return result;
}

std::string AstPrinter::getIndent() const
{
    return std::string((_indent_count - 1) * 4, SPACE);
}

void AstPrinter::validateBlockStart()
{
    if (_indent_count)
    {
        _output << "{";
        _output << endl;
    }
    ++_indent_count;
}

void AstPrinter::validateBlockEnd()
{
    --_indent_count;
    if (_indent_count)
    {
        _output << getIndent();
        _output << "}";
    }
}

void AstPrinter::printComma(size_t index)
{
    if (index)
    {
        _output << ",";
        _output << SPACE;
    }
}

void AstPrinter::visitBinaryOpNode(BinaryOpNode* node)
{
    _output << "(";
    node->left()->visit(this);
    _output << SPACE;
    _output << tokenOp(node->kind());
    _output << SPACE;
    node->right()->visit(this);
    _output << ")";
}

void AstPrinter::visitUnaryOpNode(UnaryOpNode* node)
{
    _output << tokenOp(node->kind());
    node->operand()->visit(this);
}

void AstPrinter::visitStringLiteralNode(StringLiteralNode* node)
{
    _output << "'";
    for (char symbol : node->literal())
    {
        _output << escapeChar(symbol);
    }
    _output << "'";
}

void AstPrinter::visitDoubleLiteralNode(DoubleLiteralNode* node)
{
    _output << node->literal();
}

void AstPrinter::visitIntLiteralNode(IntLiteralNode* node)
{
    _output << node->literal();
}

void AstPrinter::visitLoadNode(LoadNode* node)
{
    _output << node->var()->name();
}

void AstPrinter::visitStoreNode(StoreNode* node)
{
    _output << node->var()->name();
    _output << SPACE;
    _output << tokenOp(node->op());
    _output << SPACE;

    node->value()->visit(this);
}

void AstPrinter::visitForNode(ForNode* node)
{
    _output << "for (";
    _output << node->var()->name();
    _output << " in ";
    node->inExpr()->visit(this);
    _output << ")";
    _output << SPACE;

    node->body()->visit(this);
}

void AstPrinter::visitWhileNode(WhileNode* node)
{
    _output << "while (";
    node->whileExpr()->visit(this);
    _output << ") ";

    node->loopBlock()->visit(this);
}

void AstPrinter::visitIfNode(IfNode* node)
{
    _output << "if (";
    node->ifExpr()->visit(this);
    _output << ") ";
    node->thenBlock()->visit(this);
    if (node->elseBlock())
    {
        _output << SPACE << "else" << SPACE;
        node->elseBlock()->visit(this);
    }
}

void AstPrinter::visitBlockNode(BlockNode* node)
{
    validateBlockStart();

    const auto putSemicolon = [this](const AstNode* node)
    {
        if (!node->isWhileNode() && !node->isForNode() && !node->isIfNode())
        {
            _output << ";";
        }
    };

    for (Scope::VarIterator it = node->scope(); it.hasNext(); )
    {
        AstVar* ast_var = it.next();

        _output << getIndent();
        _output << typeToName(ast_var->type());
        _output << SPACE;
        _output << ast_var->name();
        _output << ";";
        _output << endl;
    }

    for (Scope::FunctionIterator it = node->scope(); it.hasNext(); )
    {
        FunctionNode* func_node = it.next()->node();
        
        _output << getIndent();
        visitFunctionNode(func_node);
    }
    
    for (size_t i = 0; i < node->nodes(); ++i)
    {
        AstNode* block_node = node->nodeAt(i);

        _output << getIndent();
        block_node->visit(this);
        putSemicolon(block_node);
        _output << endl;
    }

    validateBlockEnd();
}

void AstPrinter::visitFunctionNode(FunctionNode* node)
{
    _output << "function ";
    _output << typeToName(node->returnType());
    _output << SPACE;
    _output << node->name();
    _output << "(";

    for (size_t i = 0; i < node->parametersNumber(); ++i)
    {
        printComma(i);
        _output << typeToName(node->parameterType(i));
        _output << SPACE;
        _output << node->parameterName(i);
    }

    _output << ")";
    _output << SPACE;

    if (node->body()->nodes() > 0 && node->body()->nodeAt(0)->isNativeCallNode()) 
    {
        node->body()->nodeAt(0)->visit(this);
    } 
    else 
    {
        node->body()->visit(this);
    }
}

void AstPrinter::visitReturnNode(ReturnNode* node)
{
    _output << "return";
    _output << SPACE;
    node->returnExpr()->visit(this);

}

void AstPrinter::visitCallNode(CallNode* node)
{
    _output << node->name();
    _output << "(";
    for (size_t i = 0; i < node->parametersNumber(); ++i)
    {
        printComma(i);
        node->parameterAt(i)->visit(this);
    }
    _output << ")";
}

void AstPrinter::visitNativeCallNode(NativeCallNode* node)
{
    _output << "native '";
    _output << node->nativeName();
    _output << "';"; 
    _output << endl;
}

void AstPrinter::visitPrintNode(PrintNode* node)
{
    _output << "print(";
    
    for (size_t i = 0; i < node->operands(); ++i) 
    {
        printComma(i);
        node->operandAt(i)->visit(this);
    }
    
    _output << ")";
}