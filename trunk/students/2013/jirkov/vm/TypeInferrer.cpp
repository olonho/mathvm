#include "TypeInferrer.h" 

namespace mathvm {
  void Infer:: pre_node_actions( AstNode* node ) {}
  void Infer::visitFunctionNode( FunctionNode* node ) 
  { 
    const bool is_root = node == _root;
    
    make_fun_context( node, NULL );
    if (!is_root) _contexts->add_fun( node );
    embrace_args( node );
    set_type( node, VT_VOID ); 
    
    node->visitChildren( this );
    destroy_context();      
  }
  
  void Infer::visitBlockNode(BlockNode* node)
  { 
    set_type( node, VT_VOID ); 
    
    embrace_vars( node->scope() );
    
    for ( Scope::FunctionIterator it(node->scope()) ; it.hasNext(); ) 
    {
      FunctionNode* f = it.next()->node();
      visitFunctionNode(f); 
    } 
    
    
    node->visitChildren( this ); 
  }
  
  void Infer::visitCallNode(CallNode* node)
  {
    node->visitChildren( this );
    FunctionRecord* f = _contexts->find_function( node->name() );
    if ( f == NULL ) throw logic_error(std::string( "Can't derive type for call of function \"" ) + node->name() + "\" cause it was not declared\n");
    set_type( node, f->return_type() );
  }
  
  void Infer::visitReturnNode(ReturnNode* node)
  {
    node->visitChildren( this );
    set_type( node, VT_VOID );
  }
  
  void Infer::visitBinaryOpNode( BinaryOpNode* node )
  { 
    node->left()->visit( this );
    node->right()->visit( this );
    VarType fst = get_type( node->left() );
    VarType snd = get_type( node->right() );
    VarType type = VT_VOID;
    switch( node->kind() ){
      case tMUL: 
      case tADD: 
      case tSUB: 
      case tDIV:       
	type  = arithmetic_decision( fst, snd );  break;
	
      case tNEQ:
      case tEQ:       
      case tGT:       
      case tGE:       
      case tLT:       
      case tLE: 
	type = logic_decision( fst, snd ); break;
	
      case tMOD:
	type = ( fst == snd && snd == VT_INT ) ? VT_INT : VT_INVALID; break;	  
	
      case tAND:
      case tAAND:    
      case tOR:
      case tAOR:     
      case tAXOR:    
	type = (fst == snd && snd == VT_INT) ? VT_INT :  VT_INVALID; break;
      default: type = VT_INVALID; 
      
    } 
    if ( type == VT_INVALID ) 
    {
      std::cerr<< "BinaryOpNode at " << node->position() << " has invalid type! token: " << tokenStr(node->kind()) <<std::endl;
    }
    
    set_type( node, type ); 
  } 
  
  void Infer::visitUnaryOpNode( UnaryOpNode* node )
  {
    node->operand()-> visit( this);
    switch( node-> kind() )
    {
      case tSUB: set_type( node, get_type( node->operand() ) ); break;
      case tNOT: set_type( node, VT_INT ); break;
      default: throw logic_error( std::string("Can't derive type for UnaryOpNode with ") + tokenStr( node->kind() ) );
    } 
  }
  void Infer::visitStoreNode(StoreNode* node)
  {
    node-> value()-> visit( this );
    set_type( node, VT_VOID );
  }
  
  void Infer::visitLoadNode(LoadNode* node)
  { 
    set_type( node, _contexts->find_var( node->var()->name() )->type );
  }	
  
  void Infer::visitIfNode(IfNode* node)
  { 
    node->visitChildren( this ); 
    set_type( node, VT_VOID );   
  }
  
  void Infer::visitPrintNode(PrintNode* node)
  {   
    node->visitChildren( this ); 
    set_type( node, VT_VOID ); 
    
  }
  
  void Infer::visitWhileNode(WhileNode* node)
  {
    node->visitChildren( this );
    set_type( node, VT_VOID );
  }
  
  void Infer::visitForNode(ForNode* node)
  { 
    node->inExpr()->asBinaryOpNode()->visitChildren( this );
    node->body()->visit( this );
    set_type( node, VT_VOID );
  }
  
  
  
  
  
  
  void Infer::visitStringLiteralNode(StringLiteralNode* node)
  { 
    set_type( node, VT_STRING );  
  } 
  void Infer::visitDoubleLiteralNode(DoubleLiteralNode* node)
  { 
    set_type( node, VT_DOUBLE );  
  }
  void Infer::visitIntLiteralNode(IntLiteralNode* node)
  { 
    set_type( node, VT_INT ); 
  }
}  