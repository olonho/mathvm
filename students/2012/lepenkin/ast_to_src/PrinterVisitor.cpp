/* 
 * File:   PrinterVisitor.cpp
 * Author: yarik
 * 
 * Created on October 2, 2012, 6:18 PM
 */

#include "PrinterVisitor.h"


PrinterVisitor::PrinterVisitor() 
{
}

PrinterVisitor::~PrinterVisitor() 
{
}

void PrinterVisitor::init(const string& code)
{
    Status* status = _parser.parseProgram(code);
    
    if (status) 
    {
        if (status->isError())
        {    
            //cout << "Parser.parseProgram: status is an error";
            cout << status->getError() << endl;
            delete status;
            return;
        }
        else
            cout << "Parse OK!" << endl;
    }
    else
    {
        //cout << "Parser.parseProgram: returned null status" << endl;
        //cout << "Suprisingly, It's OK!" << endl;
    }
    delete status;
}

void PrinterVisitor::print(const string& str)
{
    init(str);
    PrinterVisitor printer;
    
    visitScopeAttr(_parser.top()->node()->body()->scope());
    BlockNode* topBlock = _parser.top()->node()->body();
    visitBlockNodeBody(topBlock);
}

string kindRepr(TokenKind kind) 
{
    #define KIND_TO_STRING(type, str, p) if (kind == type) return str;
    FOR_TOKENS(KIND_TO_STRING)    
    #undef KIND_TO_STRING
    return "UNDEFINED TOKEN";
}


const string typeRepr(VarType type)
{
    switch(type) 
    {
        case VT_INVALID:
            return "INVALID";
        case VT_VOID:
            return "void";
        case VT_DOUBLE:
            return "double";
        case VT_INT:
            return "int";
        case VT_STRING:
            return "string";
        default:
            return "DE";
    }
}

void PrinterVisitor::visitBinaryOpNode(BinaryOpNode* node) 
{
    cout << "(";
    node->left()->visit(this);
    cout << " " << kindRepr(node->kind()) << " ";   
    node->right()->visit(this);
    cout << ")";
}

void PrinterVisitor::visitBlockNodeBody(BlockNode* node)
{
    for (uint32_t i = 0; i < node->nodes(); ++i)
    {
        AstNode* curNode = node->nodeAt(i);
        bool cond = (curNode->isIfNode()) || (curNode->isWhileNode()) || (curNode->isForNode()) || (curNode->isReturnNode());
        curNode->visit(this);
        if (!cond) {
           cout << ";"; 
           cout << endl;
        }
    }
}

void PrinterVisitor::visitBlockNode(BlockNode* node)
{
    cout << endl << "{" << endl;
    
    visitBlockNodeBody(node);
    
    cout << "}" << endl;
}

void PrinterVisitor::visitCallNode(CallNode* node)
{
    cout << node->name() << "(";
    if (node->parametersNumber() > 0)
        node->parameterAt(0)->visit(this);
    for (uint32_t i = 1; i < node->parametersNumber(); ++i)
    {
        cout << ", ";
        node->parameterAt(i)->visit(this);
    }
    cout << ")";
    //cout << endl << "call node";
}

void PrinterVisitor::visitDoubleLiteralNode(DoubleLiteralNode* node)
{
    cout << node->literal();
}

void PrinterVisitor::visitForNode(ForNode* node)
{
    cout << endl << "for (";
    cout << node->var()->name() << " in ";    
    node->inExpr()->visit(this);
    cout << ")";
    node->body()->visit(this);
}

void PrinterVisitor::visitFunctionNode(FunctionNode* node)
{
    cout << endl << " function node ";  

}

void PrinterVisitor::visitLoadNode(LoadNode* node)
{
    cout << node->var()->name();    
}




void PrinterVisitor::visitIfNode(IfNode* node)
{
    cout << "if ";
    node->ifExpr()->visit(this);
    node->thenBlock()->visit(this);
    if (node->elseBlock())
    {
        cout << "else";
        node->elseBlock()->visit(this);
    }
}

void PrinterVisitor::visitIntLiteralNode(IntLiteralNode* node)
{
    cout << node->literal();    
}

void PrinterVisitor::visitNativeCallNode(NativeCallNode* node)
{
    cout << "native call node";    
}


void PrinterVisitor::visitPrintNode(PrintNode* node)
{
    cout << "print(";
    uint32_t n = node->operands();
    for (uint32_t i = 0; i < n; ++i)
    {
        node->operandAt(i)->visit(this);
        if (i != n - 1)
            cout << ",";
    }
    cout << ")";
}



void PrinterVisitor::visitReturnNode(ReturnNode* node)
{    
    if (node->returnExpr())
    {
        cout << "return ";
        node->returnExpr()->visit(this);
        cout << ";" << endl;    
    }        
}




void PrinterVisitor::visitStoreNode(StoreNode* node)
{
    string op = kindRepr(node->op()); 
    cout << node->var()->name() << " " << op << " ";
    node->value()->visit(this);
}


void PrinterVisitor::visitStringLiteralNode(StringLiteralNode* node)
{
    string str = node->literal();
    string out_str;
    for (uint32_t i = 0; i < str.length(); ++i) 
    {
        if (str.at(i) == '\n')
            out_str += "\\n";
        else
            out_str += str.at(i);
    }
    cout << "'" << out_str << "'";    
}


void PrinterVisitor::visitUnaryOpNode(UnaryOpNode* node)
{
    cout << kindRepr(node->kind());
    node->visitChildren(this);
}


void PrinterVisitor::visitWhileNode(WhileNode* node)
{
    cout << endl <<"while ("; 
    node->whileExpr()->visit(this);
    cout << ")";
    node->loopBlock()->visit(this);
}


void PrinterVisitor::visitScopeVars(Scope* scope)
{
    Scope::VarIterator var_iter(scope);
    while (var_iter.hasNext()) 
    {
        AstVar* curv = var_iter.next();
        cout << typeRepr(curv->type()) << " " << curv->name() << ";" << endl;
    }    
}


void PrinterVisitor::visitScopeFuns(Scope* scope)
{
    Scope::FunctionIterator fun_iter(scope);
    while (fun_iter.hasNext())
    {
        AstFunction* curf = fun_iter.next();
        cout << "function " << typeRepr(curf->returnType()) << " " << curf->name() << "(";
        if (curf->parametersNumber() > 0)
            cout << typeRepr(curf->parameterType(0)) << " " << curf->parameterName(0);
        for (uint32_t i = 1; i < curf->parametersNumber(); ++i)
            cout << "," << typeRepr(curf->parameterType(i)) << " " << curf->parameterName(i);
        cout << ")" << endl << "{" << endl;
        
        visitScopeAttr(curf->node()->body()->scope());
        visitBlockNodeBody(curf->node()->body());                
        
        cout << "}" << endl;
    }    
}


void PrinterVisitor::visitScopeAttr(Scope* scope)
{
    visitScopeFuns(scope);
    visitScopeVars(scope);
}
