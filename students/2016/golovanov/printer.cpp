#include "include/printer.h"

namespace mathvm {

PrintVisitor::PrintVisitor(std::ostream& output)
    : _nIndent(0)
    , _output(output)
{}

void PrintVisitor::visitBinaryOpNode(BinaryOpNode* node)
{
    _output << "(";
    node->left()->visit(this);
    _output << " " << tokenOp(node->kind()) << " ";
    node->right()->visit(this);
    _output << ")";
}

void PrintVisitor::visitUnaryOpNode(UnaryOpNode* node)
{
    _output << tokenOp(node->kind());
    node->operand()->visit(this);
}

void PrintVisitor::visitStringLiteralNode(StringLiteralNode* node)
{
    std::string resultStr = "";
    std::string currentStr = node->literal();

    for (size_t i = 0; i < currentStr.length(); ++i)
    {
        switch(currentStr[i])
        {
         case '\'':
            resultStr += "\\'";
            break;
         case '\\':
            resultStr += "\\\\";
            break;
         case '\n':
            resultStr += "\\n";
            break;
         case '\r':
            resultStr += "\\r";
            break;
         case '\t':
            resultStr += "\\t";
            break;
         default:
            resultStr += currentStr[i];
         }
    }

    _output << "'" << resultStr << "'";
}

void PrintVisitor::visitDoubleLiteralNode(DoubleLiteralNode* node)
{
    _output << node->literal();
}

void PrintVisitor::visitIntLiteralNode(IntLiteralNode* node)
{
    _output << node->literal();
}

void PrintVisitor::visitLoadNode(LoadNode* node)
{
    _output << node->var()->name();
}

void PrintVisitor::visitStoreNode(StoreNode* node)
{
    _output << node->var()->name()
            << " " << tokenOp(node->op()) << " ";
    node->value()->visit(this);
}

void PrintVisitor::visitForNode(ForNode* node)
{
     _output << "for (" << node->var()->name() << " in ";
     node->inExpr()->visit(this);
     _output << ") ";
     node->body()->visit(this);
}

void PrintVisitor::visitWhileNode(WhileNode* node)
{
    _output << "while (";
    node->whileExpr()->visit(this);
    _output << ") ";
    node->loopBlock()->visit(this);
}

void PrintVisitor::visitIfNode(IfNode* node)
{
    _output << "if (";
    node->ifExpr()->visit(this);
    _output << ") ";
    node->thenBlock()->visit(this);
    if (node->elseBlock())
    {
        _output << "else ";
        node->elseBlock()->visit(this);
    }
}

void PrintVisitor::visitBlockNode(BlockNode* node)
{
    if (_nIndent > 0)
    {
        _output << indent() << "{\n";
    }
    ++_nIndent;

    Scope::VarIterator varIter(node->scope());
    while(varIter.hasNext())
    {
        AstVar* var = varIter.next();
        _output << indent() << typeToName(var->type()) << " " << var->name() << ";\n";
    }

    Scope::FunctionIterator funcIter(node->scope());
    while(funcIter.hasNext())
    {
        AstFunction* func = funcIter.next();
        _output << indent();
        func->node()->visit(this);
    }

    for (size_t i = 0; i < node->nodes(); ++i)
    {
        _output << indent();
        AstNode* currenNode = node->nodeAt(i);
        currenNode->visit(this);

        if (!(currenNode->isForNode() || currenNode->isIfNode() || currenNode->isWhileNode() ||
              currenNode->isBlockNode() || currenNode->isFunctionNode()))
        {
            _output << ";\n";
        }
    }

    --_nIndent;
    if (_nIndent > 0)
    {
        _output << indent() << "}\n";
    }
}

void PrintVisitor::visitFunctionNode(FunctionNode* node)
{
    _output << "function" << " " << typeToName(node->returnType()) << " " << node->name() << "(";
    for (size_t i = 0; i < node->parametersNumber(); ++i)
    {
        if (i != 0)
        {
            _output << ", ";
        }

        _output << typeToName(node->parameterType(i)) << " " << node->parameterName(i);
    }
    _output << ") ";
    if (node->body()->nodes() > 0 && node->body()->nodeAt(0)->isNativeCallNode())
    {
        node->body()->nodeAt(0)->visit(this);
    }
    else
    {
        ++_nIndent;
        node->body()->visit(this);
        --_nIndent;
    }

}

void PrintVisitor::visitReturnNode(ReturnNode* node)
{
    _output << "return";
    if (node->returnExpr())
    {
        node->returnExpr()->visit(this);
    }
}

void PrintVisitor::visitCallNode(CallNode* node)
{
    _output <<node->name() << "(";

    for (size_t i = 0; i < node->parametersNumber(); ++i)
    {
        if (i != 0)
        {
            _output << ", ";
        }

        node->parameterAt(i)->visit(this);
    }

    _output << ")";
}

void PrintVisitor::visitNativeCallNode(NativeCallNode* node)
{
    _output << "native '" << node->nativeName() << "'\n";
}

void PrintVisitor::visitPrintNode(PrintNode* node)
{
    _output <<"print(";

    for (size_t i = 0; i < node->operands(); ++i)
    {
        if (i != 0)
        {
            _output << ", ";
        }

        node->operandAt(i)->visit(this);
    }

    _output << ")";
}


AstPrinter::AstPrinter(std::ostream& output)
    : _output(output)
{}

Status* AstPrinter::translate(const string& program, Code** code)
{
    Parser parser;
    PrintVisitor visitor(_output);

    Status* stat = parser.parseProgram(program);

    if (stat->isError())
    {
        return stat;
    }

    FunctionNode* top = parser.top()->node();
    top->body()->visit(&visitor);

    return Status::Ok();
}

Translator* Translator::create(const string& impl) {
    if (impl == "printer")
    {
        return new AstPrinter(std::cout);
    }
    else
    {
        return NULL;
    }
}

} // mathvm namespace
