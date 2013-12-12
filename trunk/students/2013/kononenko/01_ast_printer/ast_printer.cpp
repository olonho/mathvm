#include "stdafx.h"
#include "ast_printer.h"

namespace mathvm
{

ast_printer::ast_printer() 
    : indent_(0) 
{
}

ast_printer::~ast_printer() 
{
}

void ast_printer::visitBinaryOpNode(BinaryOpNode* node) 
{    
    node->left()->visit(this);    
    stream_ << " " << tokenOp(node->kind()) << " ";    
    node->right()->visit(this);      
}

void ast_printer::visitUnaryOpNode(UnaryOpNode* node) 
{
    stream_ << tokenOp(node->kind());
    node->operand()->visit(this);
}

void ast_printer::visitStringLiteralNode(StringLiteralNode* node) 
{
    string const &str = node->literal();
    stream_ << "\'";
    
    for (uint32_t i = 0; i < str.length(); ++i)
    {
        switch(str.at(i)) 
        {
        case '\\': stream_ << "\\\\"; break;
        case '\r': stream_ << "\\r" ; break;        
        case '\t': stream_ << "\\t" ; break;
        case '\n': stream_ << "\\n" ; break;
        default:   stream_ << str.at(i);
        }
    }
    
    stream_ << "\'";
}

void ast_printer::visitDoubleLiteralNode(DoubleLiteralNode* node) 
{
    stream_ << node->literal();    
}

void ast_printer::visitIntLiteralNode(IntLiteralNode* node) 
{
    stream_ << node->literal();    
}

void ast_printer::visitLoadNode(LoadNode* node) 
{
    stream_ << node->var()->name();  
}

void ast_printer::visitStoreNode(StoreNode* node) 
{
    stream_ << node->var()->name() << " " << tokenOp(node->op()) << " ";
    node->visitChildren(this);
}

void ast_printer::visitForNode(ForNode* node) 
{
    stream_ << "for (" << node->var()->name() << " in ";
    node->inExpr()->visit(this);
    stream_ << ")";

    indent();
    stream_ << "{" << endl;
    node->body()->visit(this);
    
    indent();
    stream_ << "}";
}

void ast_printer::visitWhileNode(WhileNode* node) 
{
    stream_  << "while (";
    node->whileExpr()->visit(this);
    stream_ << ")";

    indent();
    stream_ << "{" << endl;

    node->loopBlock()->visit(this);
    
    indent();
    stream_ << "}";
}

void ast_printer::visitIfNode( IfNode* node ) 
{
    stream_ <<  "if (";
    node->ifExpr()->visit(this);
    stream_ << ")";

    indent();
    stream_ << "{";
    node->thenBlock()->visit(this);

    indent();
    stream_ << "}";

    if (node->elseBlock())
    {
        indent();
        stream_ << "else" << endl;
        indent();
        stream_ << "{";

        node->elseBlock()->visit(this);
        
        indent();
        stream_ << "}";
    }
}


void ast_printer::visitBlockNode(BlockNode* node) 
{
    ++indent_;

    Scope::VarIterator var_iter(node->scope());
    bool hasVarDeclaration = var_iter.hasNext();

    
    for (Scope::VarIterator it(node->scope(), false); it.hasNext(); )
    {
        AstVar const *var = it.next();
        
        indent();
        stream_ << typeToName(var->type()) << " " << var->name() << ";";    
    }

    for (Scope::FunctionIterator it(node->scope(), false); it.hasNext(); )
    {
        AstFunction const *fn = it.next();

        indent();
        fn->node()->visit(this);
    }

    for (uint32_t i = 0; i < node->nodes(); ++i)
    {
        indent();
        node->nodeAt(i)->visit(this);
        stream_ << ";";
    }

    --indent_;    
}

void ast_printer::visitFunctionNode(FunctionNode* node) 
{
    if (is_top(node)) 
    {
        indent_ = -1;
        node->body()->visit(this);
        return;
    }

    stream_ << "function " << typeToName(node->returnType()) << " " << node->name() << "(";

    for (uint32_t i = 0; i < node->parametersNumber(); ++i)
    {
        stream_ << typeToName(node->parameterType(i)) << " " << node->parameterName(i);

        if (i != node->parametersNumber() - 1)
            stream_ << ", ";
    }

    stream_ << ")";

    indent();
    stream_ << "{";

    node->body()->visit(this);

    indent();
    stream_ << "}";
}

void ast_printer::visitReturnNode(ReturnNode* node) 
{
    stream_ << "return";
    
    if (node->returnExpr())
    {
        stream_ << " ";
        node->returnExpr()->visit(this);
    }
}

void ast_printer::visitCallNode(CallNode* node) 
{
    stream_ << node->name() << "(";

    for (unsigned int i = 0; i < node->parametersNumber(); ++i) {
        node->parameterAt(i)->visit(this);

        if (i != node->parametersNumber() - 1)
            stream_ << ", ";
    }
    
    stream_ << ")";
}

void ast_printer::visitNativeCallNode(NativeCallNode* node) 
{
}

void ast_printer::visitPrintNode(PrintNode* node) 
{
    stream_ << "print(";

    for (unsigned int i = 0; i < node->operands(); ++i) {
        node->operandAt(i)->visit(this);

        if (i != node->operands() - 1)
            stream_ << ", ";
    }
    stream_ << ")";
}

string ast_printer::print_tree(AstNode *head)
{
    stream_.clear();
    head->visit(this);
    return stream_.str();
}

void ast_printer::indent()
{
    stream_ << endl;
    for(int i = 0; i < indent_; ++i) 
        stream_ <<  "    ";
}

bool ast_printer::is_top(FunctionNode const *node)
{
    return node->name() == "<top>";
}



} // namespace mathvm;


