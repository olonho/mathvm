#include <iostream>

#include "AstPrinter.h"

namespace mathvm {

    static const char* escapeSpecials( const char c ) 
    {
        switch( c ) 
        {
            case '\n': return "\\n";
            case '\t': return "\\t";
            case '\r': return "\\r";
            case '\\': return "\\t";
            default: return NULL;
        }
    }

    static std::string escapeSpecials( std::string const& str )
    {
        std::string acc;

        for ( std::string::size_type i = 0; i != str.size(); ++i )
        {
            const char* const current = escapeSpecials( acc[i] );
            if ( current == NULL ) acc += str[i];
            else acc += current;
        }

        return acc;
    }

    void AstPrinter::visitBinaryOpNode( BinaryOpNode* node ) 
    {
        node-> left()-> visit( this );
        std::cout << ' '
            << tokenOp( node-> kind() ) 
            << ' ';
        node-> right()->visit( this );
    }

    void AstPrinter::visitUnaryOpNode( UnaryOpNode* node ) 
    {
        std::cout << tokenOp( node->kind() );
        node-> operand()-> visit( this );
    }

    void AstPrinter::visitStringLiteralNode( StringLiteralNode* node) 
    {
        std::cout << '\'' 
            << escapeSpecials( node-> literal() ) 
            << '\'';
    }

    void AstPrinter::visitDoubleLiteralNode( DoubleLiteralNode* node)
    {
        std::cout << node-> literal();
    }

    void AstPrinter::visitIntLiteralNode( IntLiteralNode* node ) 
    {
        std::cout << node-> literal();
    }

    void AstPrinter::visitLoadNode( LoadNode* node ) 
    {
        std::cout << node-> var()-> name();
    }

    void AstPrinter::visitStoreNode( StoreNode* node ) 
    {
        std::cout << node-> var()-> name() 
            << ' ' 
            << tokenOp( node-> op() ) 
            << ' ';
        node-> value()-> visit(this);
    }

    void AstPrinter::visitForNode( ForNode* node ) 
    {
        std::cout << "for (" 
            << node-> var()-> name() 
            << " in ";

        node-> inExpr()-> visit(this);

        std::cout << ")" << std::endl;

        node-> body()-> visit(this);
    }

    void AstPrinter::visitWhileNode( WhileNode* node ) 
    {
        std::cout << "while (";

        node-> whileExpr()-> visit(this);

        std::cout << ")" << std::endl;

        node-> loopBlock()-> visit(this);
    }

    void AstPrinter::visitIfNode( IfNode* node ) 
    {
        std::cout << "if (";
        node-> ifExpr()-> visit(this);
        std::cout << ")" << std::endl;
        node-> thenBlock()-> visit(this);

        if ( node-> elseBlock() ) 
        {
            std::cout << indent() << "else" << std::endl;
            node-> elseBlock()-> visit(this);

        }
    }

    void AstPrinter::goDeeper( BlockNode* node, bool indented ) 
    {
        if ( indented ) {
            std::cout << indent() << "{" << std::endl;
            widerIndent();
        }

        for( Scope::VarIterator varIterator( node->scope() ); 
                varIterator.hasNext(); 
           ) 
        {
            if ( indented )
                 std::cout << indent();

            const AstVar* const astVar = varIterator.next();
            std::cout << typeToName( astVar-> type() )
                      << ' ' << astVar-> name() 
                      << ';' << std::endl;
        
        }
        std::cout << std::endl;
       
        for ( size_t i = 0; i < node->nodes(); i++ ) 
        {
            if ( indented )
                std::cout << indent();

            node-> nodeAt(i)-> visit(this);
            std::cout << ';' << std::endl;
        }

        if ( indented ) 
        {
            tighterIndent();
            std::cout << indent() << "}" << std::endl;
        }
    }

    void AstPrinter::visitBlockNode( BlockNode* node ) 
    {
        goDeeper( node, true );
    }

    void AstPrinter::visitFunctionNode( FunctionNode* node ) 
    {
        const bool topNode = node-> position() == 0;

        if ( !topNode ) 
            std::cout << indent() << "function "
                      << typeToName( node-> returnType() ) 
                      << ' ' << node-> name() << '(';

        for ( size_t i = 0; i != node-> parametersNumber(); i++ ) 
        {
            std::cout << typeToName( node-> parameterType( i ) ) 
                      << ' ' 
                      << node->parameterName( i );

            if ( !topNode && i != node-> parametersNumber() - 1) 
                std::cout << ',' ;
        }

        if ( !topNode )
             std::cout << ")" << std::endl;
        goDeeper( node->body(), !topNode );
    }

    void AstPrinter::visitReturnNode( ReturnNode* node) 
    {
        std::cout << indent() << "return ";
        node-> returnExpr()-> visit(this);
    }

    void AstPrinter::visitCallNode( CallNode* node ) 
    {
        std::cout << node-> name() << '(';

        for ( size_t i = 0; i != node->parametersNumber(); i++) 
        {
            node-> parameterAt( i )-> visit(this);
        
            if ( i != node->parametersNumber() - 1 ) 
                std::cout << ", ";
        }
        std::cout << ')';
    }

    void AstPrinter::visitNativeCallNode( NativeCallNode* node) 
    {
        std::cout << "NativeCall" << std::endl;
    }

    void AstPrinter::visitPrintNode( PrintNode* node ) {
  
        std::cout << "print (";

        for ( size_t i = 0; i != node-> operands(); i++ ) 
        {
            node-> operandAt( i )-> visit(this);
            if ( i != node-> operands() - 1 ) 
                std::cout << ',';
        }
        std::cout << ')';
    }





}
