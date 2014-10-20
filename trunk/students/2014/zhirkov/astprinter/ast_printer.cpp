#include "../../../../include/mathvm.h"
#include "../../../../vm/parser.h"

namespace util {

    const char *escapechar(char c) {
        switch (c) {
            case '\n':
                return "\\n";
            case '\r':
                return "\\r";
            case '\t':
                return "\\t";
            case '\'':
                return "\\'";
            case '\\':
                return "\\\\";
            default:
                return NULL;
        }
    }

    std::string escape(const std::string &s) {
        std::string result;
        for (std::string::const_iterator c = s.begin(); c != s.end(); c++)
            if (escapechar(*c) == NULL) result += *c; else result += escapechar(*c);
        return result;
    }

}

namespace mathvm {

    class AstPrinterVisitor : public AstVisitor {

    private:
        std::ostream &_out;
        size_t _indent;
        const size_t _spacesForIndent;

        void indent()
        {
            for ( size_t i = 0; i < _spacesForIndent * _indent; i++ )
                _out << ' ';
        }

        void _enter()
        {
            _indent ++;
        }

        void _leave()
        {
            _indent--;
        }

        void functionDeclaration( Scope *scope )
        {
            Scope::FunctionIterator iter( scope );
            while ( iter.hasNext() )
                iter.next()-> node()-> visit( this );

        }

        void variableDeclaration( Scope *scope )
        {
            Scope::VarIterator iter( scope );
            while ( iter.hasNext() ) {
                AstVar &x = *( iter.next() );
                indent();
                _out <<
                        typeToName(x.type()) <<
                        " " <<
                        x.name() <<
                        ";" <<
                        std::endl;
            }
        }


    public:


        /* ctor */
        AstPrinterVisitor(
                std::ostream &out = std::cout,
                const uint8_t indentSpaces = 3 ):
                _out(out),
                _indent(0),
                _spacesForIndent(indentSpaces) { }

        void enterBlock(BlockNode *node)
        {
            variableDeclaration( node->scope() );
            functionDeclaration( node->scope() );

            for ( uint32_t i = 0; i < node->nodes(); i++ ) {
                indent();
                AstNode& current = *( node->nodeAt(i) ); //I d think it should be size_t -_-
                current.visit( this );

                //hacky
                if ( current.isCallNode() ) _out << ';';
                _out << endl;
            }
        }

        virtual void visitBinaryOpNode( BinaryOpNode *node )
        {
            _out << '(';
            node-> left()-> visit(this);
            _out << ' ' << tokenOp( node-> kind() ) << ' ';
            node-> right()-> visit(this);
            _out << ')';
        }

        virtual void visitUnaryOpNode( UnaryOpNode *node )
        {
            _out << tokenOp( node->kind() ) << ' ';
            node-> operand()-> visit( this );
        }

        virtual void visitStringLiteralNode( StringLiteralNode *node )
        {
            _out << '\'' << util::escape( node->literal() ) << '\'';
        }

        virtual void visitDoubleLiteralNode( DoubleLiteralNode *node )
        {
            _out << node-> literal();
        }

        virtual void visitIntLiteralNode( IntLiteralNode *node )
        {
            _out << node-> literal();
        }

        virtual void visitLoadNode( LoadNode *node )
        {
            _out << node-> var()-> name();
        }

        virtual void visitStoreNode( StoreNode *node )
        {
            _out << node-> var()-> name() << ' '
                    << tokenOp( node->op() ) << ' ';
            node-> value()-> visit( this );
            _out << ';';
        }

        virtual void visitForNode( ForNode *node )
        {
            _out << "for ("
                    << node->var()->name()
                    << " in ";
            node-> inExpr()->visit( this );
            _out << ')';
            node-> body()-> visit( this );
        }

        virtual void visitWhileNode( WhileNode *node )
        {
            _out << "while (";
            node-> whileExpr()-> visit( this );
            _out << ") ";
            node-> loopBlock()-> visit( this );
        }

        virtual void visitIfNode( IfNode *node )
        {
            _out << "if (";
            node-> ifExpr()-> visit( this );
            _out << ") ";
            node-> thenBlock()-> visit( this );
            if ( node-> elseBlock() != NULL )
            {
                _out << " else ";
                node-> elseBlock()-> visit( this );
            }
        }

        virtual void visitBlockNode( BlockNode *node )
        {
            _out << endl;
            indent();
            _out << '{';
            _enter();
            _out << endl;

            enterBlock(node);
            _leave();
            indent();
            _out << '}' << endl;
        }

        virtual void visitNativeCallNode( NativeCallNode *node )
        {
            _out << " native '" << node-> nativeName()
                    << "';" << endl;
        }

        virtual void visitFunctionNode( FunctionNode *node )
        {
            if ( node-> name() != AstFunction::top_name) {
                _out << "function "
                        << typeToName(node->returnType()) << ' '
                        << node-> name()
                        << '(';
                for ( uint32_t i = 0; i < node->parametersNumber(); i++) // IT SHOULD BE size_t, shouldn't it?
                {
                    if (i != 0)
                        _out << ", ";
                    _out << typeToName(node->parameterType(i))
                            << " " << node->parameterName(i);
                }
                _out << ")";
            }
            if (node-> body()-> nodeAt(0)-> isNativeCallNode())
                visitNativeCallNode( node-> body()-> nodeAt(0)-> asNativeCallNode() );
            else {
                node-> body()-> visit(this);
                _out << endl;
            }
        }

        virtual void visitReturnNode( ReturnNode *node )
        {
            _out << "return";
            if ( node-> returnExpr() != NULL ) {
                _out << ' ';
                node-> returnExpr()->visit( this );
            }
            _out << ';';
        }

        virtual void visitCallNode( CallNode *node )
        {
            _out << node-> name() << '(';
            for ( uint32_t i = 0; i < node->parametersNumber(); i++ )
            {
                if (i != 0) _out << ", ";
                node-> parameterAt( i )-> visit( this );
            }
            _out << ')';
        }

        virtual void visitPrintNode( PrintNode *node )
        {
            _out << "print (";
            for ( uint32_t i = 0; i < node-> operands(); i++ ) {
                if (i != 0) _out << ", ";
                node-> operandAt( i )-> visit( this );
            }
            _out << ");";
        }

    };

    class AstPrinter : public Translator {
    public:
        virtual Status *translate( const string &program, Code **code ) {
            Parser parser;
            Status *status = parser.parseProgram( program );
            if ( status != NULL ) return status;

            AstPrinterVisitor printer;
            printer.enterBlock(parser.top()->node()->body());

            return new Status();
        }
    };

    Translator *Translator::create(const string &impl) {
        if (impl == "printer") {
            return new AstPrinter();
        } else {
            return NULL;
        }
    }

}
