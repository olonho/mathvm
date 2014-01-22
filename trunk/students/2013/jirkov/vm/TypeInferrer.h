#pragma once
#include <map>

#include "../../../../include/ast.h"
#include "../../../../include/mathvm.h"
#include "../../../../include/visitors.h"
#include "../../../../vm/parser.h" 

#include "helpers.h"
#include "AbstractInterpreter.h"
#include "Annotation.h"

namespace mathvm{
  
  
  class Infer : public AbstractInterpreter { 
    FunctionNode* _root;
  public:
    void run() { visitFunctionNode( _root ); }
    Infer( FunctionNode* node ) : _root( node ) {}   
    
    
    void pre_node_actions( AstNode* node );
    
    void visitFunctionNode( FunctionNode* node );

    void visitIntLiteralNode( IntLiteralNode* node );
    void visitDoubleLiteralNode( DoubleLiteralNode* node );
    void visitStringLiteralNode( StringLiteralNode* node );
    
    void visitBlockNode( BlockNode* node );
    
    void visitPrintNode( PrintNode* node );
    
    void visitIfNode( IfNode* node );
    
    void visitLoadNode( LoadNode* node );
    void visitStoreNode( StoreNode* node );
    
    void visitBinaryOpNode( BinaryOpNode* node );
    void visitUnaryOpNode( UnaryOpNode* node );
    
    void visitReturnNode(  ReturnNode* node );
    void visitCallNode( CallNode* node );

    void visitWhileNode( WhileNode* node );
    void visitForNode( ForNode* node );
        
    /*DO(ForNode, "for")                  \
     *           DO(WhileNode, "while")              \ 
     *     
     *           DO(NativeCallNode, "native call")   \
     */
    
    private:
      static VarType arithmetic_decision( VarType fst, VarType snd )
      {
	if ( fst == VT_INT && snd == VT_INT) return VT_INT;
	if ( fst == VT_DOUBLE && snd == VT_INT ) return VT_DOUBLE;
	if ( fst == VT_INT && snd == VT_DOUBLE ) return VT_DOUBLE;
	if ( fst == VT_DOUBLE && snd == VT_DOUBLE ) return VT_DOUBLE;
	std::cerr<< "arithmetic decision fails for :" << typeToName( fst ) << " and " << typeToName( snd ) << std::endl;
	return VT_INVALID;
      }
      
      static VarType logic_decision( VarType fst, VarType snd )
      {
	if ( ( fst == VT_INT || fst == VT_DOUBLE ) && ( snd == VT_INT  || snd == VT_DOUBLE ) ) return VT_INT;
	return VT_INVALID;
      } 
      
  };
  
  struct Cleaner : public AstBaseVisitor {
    Cleaner( FunctionNode* node ) { visitFunctionNode( node ); } 
    #define CLEAN(name, skip) virtual void visit##name( name* node ) { node->visitChildren( this ); delete get_annotation( node );}
    FOR_NODES( CLEAN )
    #undef CLEAN
  };
  
  
  
}