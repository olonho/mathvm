#include "ast_pretty_printer.h"


using namespace mathvm;


AstPrettyPrinterVisitor::Printer & AstPrettyPrinterVisitor::Printer::operator<<(const string & value)
{
    cout << value;
    return *this;
}

AstPrettyPrinterVisitor::Printer & AstPrettyPrinterVisitor::Printer::operator<<(int64_t value)
{
    cout << value;
    return *this;
}

AstPrettyPrinterVisitor::Printer & AstPrettyPrinterVisitor::Printer::operator<<(double value)
{
    cout << std::to_string(value);
    return *this;
}

AstPrettyPrinterVisitor::Printer & AstPrettyPrinterVisitor::Printer::operator<<(VarType value)
{
    switch(value) 
    {
        case VT_INVALID : 
            *this << "<invalid type>";
            break;
        case VT_VOID : 
            *this << "void";
            break;
        case VT_DOUBLE :
            *this << "double";
            break;
        case VT_INT : 
            *this << "int";
            break;
        case VT_STRING : 
            *this << "string";
            break;
    }
    return *this;
}

AstPrettyPrinterVisitor::Printer & AstPrettyPrinterVisitor::Printer::operator<<(const AstVar * value)
{
    *this << value->type() << " " << value->name();
    return *this;
}

AstPrettyPrinterVisitor::Printer & AstPrettyPrinterVisitor::Printer::operator<<(Special value)
{
    switch(value) 
    {
        case SEMICOLON_ENDL : 
            if (!skipNextSemicolon) 
            {
                *this << ";";
            }
            *this << "\n";
            skipNextSemicolon = false;            
            break;
        case INC_INDENT :
            ++indentSize;
            break;
        case DEC_INDENT :
            --indentSize;
            break;
        case SKIP_NEXT_SEMICOLON :
            skipNextSemicolon = true;
            break;
        case INDENT :
            for (int i = 0; i < indentSize * 4; ++i) 
            {
                *this << " ";
            }
            break;
    }
    
    return *this;
}

AstPrettyPrinterVisitor::Printer & AstPrettyPrinterVisitor::Printer::operator<<(TokenKind value)
{
    string defaultString = "<unexpected token>";
    switch(value) 
    {
        #define ENUM_ELEM(t, s, p) \
        case t : { \
            *this << ((p == 0 && t != tNOT) ? defaultString : s); \
            break; \
        }  
        FOR_TOKENS(ENUM_ELEM)
        #undef ENUM_ELEM
        case tTokenCount : *this << defaultString;
    }
    return *this;
}

void AstPrettyPrinterVisitor::visitPrintNode(PrintNode * node) 
{    
    out << "print(";
    for(uint16_t i = 0; i < node->operands(); ++i) 
    {
        node->operandAt(i)->visit(this);
        if (i != node->operands() - 1) 
        {
            out << ", ";
        }
    }
    out << ")";
}

void AstPrettyPrinterVisitor::visitNativeCallNode(NativeCallNode * node) 
{  
    out << "native '" << node->nativeName() << "'";
}

void AstPrettyPrinterVisitor::visitCallNode(CallNode * node) 
{
    out << node->name() << "(";
    for(uint16_t i = 0; i < node->parametersNumber(); ++i) 
    {
        node->parameterAt(i)->visit(this);
        if (i != node->parametersNumber() - 1) 
        {
            out << ", ";
        }
    }
    out << ")";
}

void AstPrettyPrinterVisitor::visitReturnNode(ReturnNode * node) 
{
    out << "return ";
    node->visitChildren(this);
}

void AstPrettyPrinterVisitor::visitFunctionNode(FunctionNode * node) 
{
    out << "function " << node->returnType() << " " << node->name() << "(";
    for (uint32_t i = 0; i < node->parametersNumber(); ++i) 
    {
        out << node->parameterType(i) << " " << node->parameterName(i);
        if (i != node->parametersNumber() - 1) 
        {
            out << ", ";
        }
    }
    out << ")";

    BlockNode * body = node->body();
    if (body->nodeAt(0)->isNativeCallNode()) 
    {
        body->nodeAt(0)->visit(this);
    }
    else 
    {
        out << "{\n" << INC_INDENT;
        body->visit(this);
        out << DEC_INDENT << INDENT << "}" << SKIP_NEXT_SEMICOLON;
    }
}

void AstPrettyPrinterVisitor::visitBlockNode(BlockNode * node) 
{
    Scope::VarIterator vars(node->scope());
    while (vars.hasNext()) {
        AstVar * var = vars.next();
        out << INDENT << var->type() << " " << var->name() << SEMICOLON_ENDL;
    }

    Scope::FunctionIterator funs(node->scope());
    while (funs.hasNext()) {
        out << INDENT;
        funs.next()->node()->visit(this);
        out << SEMICOLON_ENDL;
    }

    for (uint32_t i = 0; i < node->nodes(); ++i) 
    {
        out << INDENT;
        node->nodeAt(i)->visit(this);
        out << SEMICOLON_ENDL;
    }
}

void AstPrettyPrinterVisitor::visitIfNode(IfNode * node) 
{
    out << "if (";
    node->ifExpr()->visit(this);
    out << ") {\n" << INC_INDENT;
    node->thenBlock()->visit(this);
    out << DEC_INDENT << INDENT << "}";
    if (node->elseBlock() != nullptr) 
    {
        out << INDENT << " else {\n" << INC_INDENT;
        node->elseBlock()->visit(this);
        out << DEC_INDENT << INDENT << "}";
    }
    out << SKIP_NEXT_SEMICOLON;
}

void AstPrettyPrinterVisitor::visitWhileNode(WhileNode * node) 
{ 
    out << "while (";
    node->whileExpr()->visit(this);
    out << ") {\n" << INC_INDENT;
    node->loopBlock()->visit(this); 
    out << DEC_INDENT << INDENT << "}" << SKIP_NEXT_SEMICOLON;
}

void AstPrettyPrinterVisitor::visitForNode(ForNode * node) 
{
    out << "for (" << node->var()->name() << " in ";
    node->inExpr()->visit(this);
    out << ") {\n" << INC_INDENT;
    node->body()->visit(this); 
    out << DEC_INDENT << INDENT << "}" << SKIP_NEXT_SEMICOLON;
}

void AstPrettyPrinterVisitor::visitStoreNode(StoreNode * node) 
{
    out << node->var()->name() << " " << node->op() << " ";
    node->value()->visit(this);
}

void AstPrettyPrinterVisitor::visitLoadNode(LoadNode * node) 
{
    out << node->var()->name();
}

void AstPrettyPrinterVisitor::visitIntLiteralNode(IntLiteralNode * node) 
{
    out << node->literal();
}

void AstPrettyPrinterVisitor::visitDoubleLiteralNode(DoubleLiteralNode * node) 
{
    out << node->literal();
}

string AstPrettyPrinterVisitor::escaped(const string & value) 
{
    string result = "";
    for (char c: value) {
        switch(c) {
            case '\n' : 
            result += "\\n";
            break;
            case '\t' : 
                result += "\\t";
                break;
            case '\r' : 
                result += "\\r";
                break;
            case '\\' : 
                result += "\\\\";
                break;
            default : result += c;
        }
    }
    return result;
} 

void AstPrettyPrinterVisitor::visitStringLiteralNode(StringLiteralNode * node) 
{
    out << "'" << escaped(node->literal()) << "'";    
}

void AstPrettyPrinterVisitor::visitUnaryOpNode(UnaryOpNode * node) 
{
    out << node->kind();   
    node->operand()->visit(this); 
}

void AstPrettyPrinterVisitor::visitBinaryOpNode(BinaryOpNode * node) 
{
    out << "(";   
    node->left()->visit(this);  
    out << " " << node->kind() << " ";   
    node->right()->visit(this);       
    out << ")";     
}

