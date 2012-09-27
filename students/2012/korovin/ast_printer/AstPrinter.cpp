#include "AstPrinter.h"
#include <map>

std::string escape(std::string const &s)
{
    std::size_t n = s.length();
    std::string escaped;
    escaped.reserve(n * 2);

    for (std::size_t i = 0; i < n; ++i) {
        switch(s[i]) {
        case '\n':
            escaped += "\\n";
            break;
        case '\r':
            escaped += "\\r";
            break;
        case '\t':
            escaped += "\\t";
            break;
        case '\\':
            escaped += "\\\\";
            break;
        default:
            escaped += s[i];
            break;
        }
    }
    return escaped;
}

namespace mathvm {

AstPrinter::AstPrinter() : output_(&std::cout) {
};

void AstPrinter::visitBinaryOpNode( BinaryOpNode* node )
{
    *output_ << "(";
    node->left()->visit(this);
    *output_ << " " << tokenOp(node->kind()) << " ";
    node->right()->visit(this);
    *output_ << ")";
}

void AstPrinter::visitUnaryOpNode( UnaryOpNode* node )
{
    *output_ << tokenOp(node->kind());
    node->operand()->visit(this);
}

void AstPrinter::visitStringLiteralNode( StringLiteralNode* node )
{
    *output_ << "'" << escape(node->literal()) << "'";
}

void AstPrinter::visitDoubleLiteralNode( DoubleLiteralNode* node )
{
    *output_ << node->literal();
}

void AstPrinter::visitIntLiteralNode( IntLiteralNode* node )
{
    *output_ << node->literal();
}

void AstPrinter::visitLoadNode( LoadNode* node )
{
    *output_ << node->var()->name();
}

void AstPrinter::visitStoreNode( StoreNode* node )
{
    *output_ << node->var()->name();
    *output_ << " " << tokenOp(node->op()) << " ";
    node->visitChildren(this);
}

void AstPrinter::visitForNode( ForNode* node )
{
    *output_ << "for (" << node->var()->name() << " in ";
    node->inExpr()->visit(this);
    *output_ << ")";
    node->body()->visit(this);
}

void AstPrinter::visitWhileNode( WhileNode* node )
{
    *output_ << "while (";
    node->whileExpr()->visit(this);
    *output_ << ")";
    node->loopBlock()->visit(this);
}

void AstPrinter::visitIfNode( IfNode* node )
{
    *output_ << "if (";
    node->ifExpr()->visit(this);
    *output_ << ")";
    node->thenBlock()->visit(this);

    if(node->elseBlock()) {
        *output_ << " else ";
        node->elseBlock()->visit(this);
    }
}

void AstPrinter::visitBlockNode( BlockNode* node )
{
    *output_ << "{" << std::endl;
    blockContents(node);
    *output_ << "}" << std::endl;
}

void AstPrinter::visitFunctionNode( FunctionNode* node )
{
    *output_ << "function " << typeToName(node->returnType()) << " " 
             << node->name() ;
    *output_ << "(";
    for (size_t i = 0; i < node->parametersNumber(); ++i) {
        *output_ << typeToName(node->parameterType(i)) << " " 
                 << node->parameterName(i) ;
        *output_ << ((i != node->parametersNumber() - 1) ? ", " : "");
    }
    *output_ << ")";

    if (node->body()->nodeAt(0)->isNativeCallNode()) {
        node->body()->nodeAt(0)->visit(this);
        *output_ << std::endl;
    } else {
        node->body()->visit(this);
    }
}

void AstPrinter::visitReturnNode( ReturnNode* node )
{
    *output_ << "return ";
    node->visitChildren(this);
}

void AstPrinter::visitCallNode( CallNode* node )
{
    *output_ << node->name() ;
    *output_ << "(";
    for(size_t i = 0; i < node->parametersNumber(); ++i) {
        node->parameterAt(i)->visit(this);
        *output_ << ((i != node->parametersNumber() - 1) ? ", " : "");
    }
    *output_ << ")";
}

void AstPrinter::visitNativeCallNode( NativeCallNode* node )
{
    *output_ << " native '" << node->nativeName() << "';";
}

void AstPrinter::visitPrintNode( PrintNode* node )
{
    *output_ << "print" ;
    *output_ << "(";
    for (size_t i = 0; i < node->operands(); ++i) {
        node->operandAt(i)->visit(this);
        *output_ << ((i != node->operands() - 1) ? ", " : "");
    }
    *output_ << ")";
}

void AstPrinter::functionDeclarations( Scope* scope )
{
    Scope::FunctionIterator fNative(scope);
    vector<AstFunction*> delayedDeclarations;
    while(fNative.hasNext()) {
        AstFunction* func = fNative.next();
        if (func->node()->body()->nodeAt(0)->isNativeCallNode())
            func->node()->visit(this);
        else
            delayedDeclarations.push_back(func);
    }

    for(size_t i = 0; i < delayedDeclarations.size(); ++i) {
        delayedDeclarations[i]->node()->visit(this);
    }
}

void AstPrinter::variableDeclarations( Scope* scope )
{
    Scope::VarIterator vIter(scope);
    while(vIter.hasNext()) {
        AstVar* variable = vIter.next();  
        *output_ << typeToName(variable->type()) << " " 
                 << variable->name() << ";" << std::endl;
    }
}

void AstPrinter::blockContents(BlockNode* node) 
{
    // Functions and variables
    variableDeclarations(node->scope());
    functionDeclarations(node->scope());

    // Code part
    for(size_t i = 0; i < node->nodes(); ++i) {
        node->nodeAt(i)->visit(this);
        if(!node->nodeAt(i)->isIfNode() && !node->nodeAt(i)->isForNode()  
           && !node->nodeAt(i)->isWhileNode()) {
            *output_ << ";";
        }
        *output_ << std::endl;
    }
}

}