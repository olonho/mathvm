#include "source_printer.h"
#include <iostream>

using std::cout;
using std::endl;

bool needSemicolon = true;

void print_escape( std::string const &str, std::ostream & out ) {
    out << "\'";
    for (unsigned int i = 0; i < str.length(); ++i)
        switch(str[i]) {
        case '\\':
            out << "\\\\";
            break;
        case '\r':
            out << "\\r";
            break;        
        case '\t':
            out << "\\t";
            break;
        case '\n':
            out << "\\n";
            break;
        default:
            out << str[i];
            break;
        }
    out << "\'";
}


source_printer::source_printer( std::ostream & out_stream ) : my_ostream(out_stream), my_indent(0) {}

source_printer::~source_printer() {}

string source_printer::get_indent_line() {
     string indent_line = "";
    for(int i = 0; i < this->my_indent; ++i) {
        indent_line += "    ";
    }
    return indent_line;
}

void source_printer::visitBinaryOpNode( BinaryOpNode* node ) {    
   
    node->left()->visit(this);    
    
    my_ostream << " " << tokenOp(node->kind()) << " ";    
    
    node->right()->visit(this);      
}

void source_printer::visitUnaryOpNode( UnaryOpNode* node ) {
   my_ostream << tokenOp(node->kind());
   node->operand()->visit(this);
}

void source_printer::visitStringLiteralNode( StringLiteralNode* node ) {
    print_escape(node->literal(), my_ostream);
}

void source_printer::visitDoubleLiteralNode( DoubleLiteralNode* node ) {
    my_ostream << node->literal();    

}

void source_printer::visitIntLiteralNode( IntLiteralNode* node ) {
    my_ostream << node->literal();    
}

void source_printer::visitLoadNode( LoadNode* node ) {
    my_ostream << node->var()->name();  
}

void source_printer::visitStoreNode( StoreNode* node ) {
    my_ostream << node->var()->name() << " " << tokenOp(node->op()) << " ";
    node->visitChildren(this);
    needSemicolon = true;
}

void source_printer::visitForNode( ForNode* node ) {
    my_ostream << "for (" << node->var()->name() << " in ";
    node->inExpr()->visit(this);
    my_ostream << ") {" << endl;

    node->body()->visit(this);
    
    my_ostream << "}";
    my_ostream << endl;
    needSemicolon = false;
}

void source_printer::visitWhileNode( WhileNode* node ) {
    
    my_ostream  << "while (";
    node->whileExpr()->visit(this);
    my_ostream << ") {" << endl;
    
    node->loopBlock()->visit(this);
    my_ostream << "}";
    my_ostream << endl;
    needSemicolon = false;
}

void source_printer::visitIfNode( IfNode* node ) {
    
    my_ostream <<  "if (";
    node->ifExpr()->visit(this);
    my_ostream << ") {" << endl;

    node->thenBlock()->visit(this);
    my_ostream << get_indent_line() << "}";
    needSemicolon = false;

    if (node->elseBlock() == 0) {
        my_ostream << endl;
        return;
    }
    
    my_ostream << " else {" << endl;    
    node->elseBlock()->visit(this);
    my_ostream << get_indent_line() << "}";
    my_ostream << endl;
    needSemicolon = false;
    
}


void source_printer::visitBlockNode( BlockNode* node ) {
    //Border border("blockNode", my_ostream);
    ++my_indent;
    Scope::VarIterator var_iter(node->scope());
    bool hasVarDeclaration = var_iter.hasNext();

    while(var_iter.hasNext()) {
        AstVar const * var = var_iter.next();
        my_ostream << get_indent_line() << typeToName(var->type()) << " " << var->name() <<";" << endl;    
    }

    if(hasVarDeclaration)
        my_ostream << endl;

    Scope::FunctionIterator func(node->scope());
    while(func.hasNext()) func.next()->node()->visit(this);
    for (uint32_t i = 0; i < node->nodes(); ++i)
    {
        my_ostream << get_indent_line();
    	node->nodeAt(i)->visit(this);

	    if (needSemicolon) {
		    my_ostream << ";";
	    }        

        my_ostream << endl;
        needSemicolon = true;
    }   
    --my_indent;    
}

void source_printer::visitFunctionNode( FunctionNode* node ) {
    if(node->name() == "<top>") {
        my_indent = -1;
	    node->body()->visit(this);
	    return;
    }

    my_ostream << endl;
    my_ostream << get_indent_line() << "function " << typeToName(node->returnType()) << " " << node->name() << "(";
    
    for (unsigned int j = 0; j < node->parametersNumber(); ++j) {
        if (j > 0) my_ostream << ", ";

        my_ostream << typeToName(node->parameterType(j)) << " " << node->parameterName(j);
    }

    my_ostream << ") ";

    if (node->body()->nodes() > 0 && node->body()->nodeAt(0)->isNativeCallNode()) {
	    node->body()->nodeAt(0)->visit(this);
    } else {
       	my_ostream << "{" << endl;

    	node->body()->visit(this);

    	my_ostream << "}" << endl;
        needSemicolon = false;
    }
}

void source_printer::visitReturnNode( ReturnNode* node ) {
    needSemicolon = false;
    
    if(node->returnExpr() != 0) {
        my_ostream << "return ";
        node->returnExpr()->visit(this);
        needSemicolon = true;
    }        
}

void source_printer::visitCallNode( CallNode* node ) {
    my_ostream << node->name() << "(";

    for (unsigned int i = 0; i < node->parametersNumber(); ++i) {
        if (i > 0) my_ostream << ", ";
        node->parameterAt(i)->visit(this);
    }
    my_ostream << ")";
    needSemicolon = true;
}

void source_printer::visitNativeCallNode( NativeCallNode* node ) {
    my_ostream << "native '"<< node->nativeName() << "';" << endl;
}


void source_printer::visitPrintNode( PrintNode* node ) {
    my_ostream << "print(";

    for (unsigned int i = 0; i < node->operands(); ++i) {
        if (i > 0) my_ostream << ", ";
        AstNode *pNode = node->operandAt(i);
        pNode->visit(this);
    }
    my_ostream << ")";
    needSemicolon = true;
}




