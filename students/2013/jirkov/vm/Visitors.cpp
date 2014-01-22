#include "Translator.h"
#include "TypeInferrer.h"
#include "AbstractInterpreter.h"
#include "Translator.h"

namespace mathvm {
  
  void MVMTranslator::pre_node_actions( AstNode* node ) {
    //     ins( BC_DUMP );
  }
  
  void MVMTranslator::visitFunctionNode( FunctionNode* node )
  {
    //     pre_node_actions( node );
    bool is_root = node == _root;
    
    FunctionRecord* rec = new FunctionRecord( is_root? "" : node->name(), node->signature(), false );
    make_fun_context( node, rec );
    
    if (!is_root) _contexts->functions.insert(make_pair(node->name(), rec));
    
    _functions.push_back( rec );  
    _contexts->add_fun( rec ); 
    
    embrace_args( node );
    
    visitBlockNode( node-> body() );
    if (is_root) ins( BC_STOP );
    
    std::cout<< "Function " << node->name() << std::endl;
    _contexts->topmost_function_record()->code.dump( std::cout );
    destroy_context();
  }
  
  
  void MVMTranslator::visitBlockNode( BlockNode* node )
  { 
    pre_node_actions( node );
    
    make_context();
    embrace_vars( node-> scope() );
    
    
    for ( Scope::FunctionIterator it(node->scope()) ; it.hasNext(); ) 
    {
      FunctionNode* f = it.next()->node();
      visitFunctionNode(f); 
    } 
    
    for ( size_t i = 0; i < node-> nodes(); ++i ) 
    { 
      AstNode& child = *( node-> nodeAt( i ) );
      
      child.visit( this );
      
      if ( child.isCallNode() &&  _contexts-> find_function( child.asCallNode()->name() )-> is_void() )
	ins( BC_POP ); 
    }
    destroy_context();
  }
  
  
  void MVMTranslator::visitDoubleLiteralNode( DoubleLiteralNode * node ) {
    pre_node_actions( node );
    push_dvalue( node->literal() );
  }
  
  void MVMTranslator::push_ivalue( const int64_t value )
  {
    switch( value ) {
      case 0:
	ins( BC_ILOAD0 ); break;
      case 1:
	ins( BC_ILOAD1 ); break;
      case -1:
	ins( BC_ILOADM1 ); break;
      default:
	ins64( BC_ILOAD, value );
    }   
  }
  void MVMTranslator::push_dvalue( const double value )
  {
    if ( value == 0.0 ) ins( BC_DLOAD0 );
    else ins64( BC_DLOAD, value ); 
  }
  
  void MVMTranslator::visitIntLiteralNode( IntLiteralNode * node ) {
    pre_node_actions( node );
    push_ivalue( node->literal());
  }
  
  void MVMTranslator::visitStringLiteralNode( StringLiteralNode* node )
  {
    pre_node_actions( node );
    uint16_t id = constant_pool.vivify( node->literal() );
    if ( node-> literal() == "" ) ins( BC_SLOAD0 );
    else ins16( BC_SLOAD, id );
    //     push( VT_STRING );
  }
  
  
  void MVMTranslator::visitPrintNode( PrintNode* node )
  {  
    for ( size_t i = 0; i < node-> operands(); ++i )
    {
      AstNode* child =  node->operandAt( i );
      child-> visit( this );
      switch( get_type( child ) )
      {
	case VT_INT: ins( BC_IPRINT ); break;
	case VT_DOUBLE: ins( BC_DPRINT ); break;
	case VT_STRING: ins( BC_SPRINT ); break;
	default: throw logic_error( "attempt to insert a printing code for unsupported data format!" );
      } 
    }
  }
  
  void MVMTranslator::visitLoadNode( LoadNode* node )
  { 
    pre_node_actions( node );
    const AstVar* const astVar = node-> var();
    load_var( _contexts-> find_var( astVar-> name() ) );
  }
  
  
  void MVMTranslator::visitStoreNode( StoreNode* node )
  {
    pre_node_actions( node );
    AstVar const* astVar = node-> var();
    const Variable* var = _contexts-> find_var( astVar-> name() );
    
    switch( node-> op() )
    {
      case tASSIGN: {//it would be so cool if we could omit the last bytecode we pushed, right? -__-
	node-> value()-> visit( this );
	get_conversion_vector( get_type( node->value() ), var->type );
	break;
      }
      case tINCRSET: 
	
	load_var( var );
	node-> value()-> visit( this );
	get_conversion_vector( get_type( node->value() ), var->type );
	
	switch( var-> type)
	{
	  case VT_INT: ins( BC_IADD ); break;
	  case VT_DOUBLE: ins( BC_DADD ); break;
	  default: throw logic_error( "Invalid type for INCRSET operation!" );
	}; 
	break;
	
	
	  case tDECRSET:
	    load_var( var );
	    node-> value()-> visit( this );
	    get_conversion_vector( get_type( node->value() ), var->type );
	    switch( var-> type )
	    {
	      case VT_INT: ins( BC_ISUB ); break;
	      case VT_DOUBLE: ins( BC_DSUB ); break;
	      default: throw logic_error( "Invalid type for INCRSET operation!" );
	    }; 
	    break;
	    
	      default: throw logic_error( "Unknown token inside StoreNode" );
    }
    
    store_var( var );
    
  } 
  
  static void add_all( std::vector<Instruction>& where, std::vector<Instruction> what ) 
  {
    for( std::vector<Instruction>::iterator it = what.begin(); it != what.end(); it++)
      where.push_back( *it );
  }
  
  
  bool MVMTranslator::handle_arithmetic( TokenKind kind, VarType fst, VarType snd, std::vector<Instruction>& fst_conv, std::vector<Instruction>& snd_conv, std::vector<uint8_t>& result )
  {
    Instruction instr;
    const bool need_widening = (( fst == VT_DOUBLE ) || (snd == VT_DOUBLE ) ) && (( fst == VT_INT ) || (snd == VT_INT ) );
    const bool wide = fst == VT_DOUBLE || snd == VT_DOUBLE;
    
    switch( kind )
    { 
      case tADD: instr = wide ? BC_DADD: BC_IADD; break; 
      case tMUL: instr = wide ? BC_DMUL: BC_IMUL; break;
      case tDIV: instr = wide ? BC_DDIV: BC_IDIV; break;
      case tSUB: instr = wide ? BC_DSUB: BC_ISUB; break;
      default: return false;
    } 
    result.push_back( instr );
    
    if ( need_widening )
    {
      add_all( fst_conv, MVMTranslator::get_conversion_vector( fst, VT_DOUBLE ).first);
      add_all( snd_conv, MVMTranslator::get_conversion_vector( snd, VT_DOUBLE ).first);
    }
    
    return true;
  }
  bool MVMTranslator::handle_booleans( TokenKind kind, VarType fst, VarType snd, std::vector<Instruction>& fst_conv, std::vector<Instruction>& snd_conv, std::vector<uint8_t>& result )
  {
    Instruction instr = BC_INVALID;
    switch( kind )
    {
      case tAND: instr =  BC_IMUL;   break;
      case tAAND: instr =  BC_IAAND; break;
      case tAOR: instr =  BC_IAOR ;  break;
      case tAXOR: instr =  BC_IAXOR; break;
      case tOR: instr =  BC_IAOR;    break;
      
      default: return false;
    }
    if ( instr != BC_INVALID )
    {
      add_all(fst_conv, get_conversion_vector(fst, VT_INT).first );
      add_all(snd_conv, get_conversion_vector(snd, VT_INT).first );
    }
    result.push_back( instr );
    return true;    
  }
  
  bool MVMTranslator::handle_logic( TokenKind kind, VarType fst, VarType snd, std::vector<Instruction>& fst_conv, std::vector<Instruction>& snd_conv, std::vector<uint8_t>& result )
  { 
    if ( kind != tNEQ && kind != tEQ &&  kind != tGT && kind != tGE  && kind != tLT &&  kind != tLE ) return false; 
    const bool wides = fst == VT_DOUBLE && snd == VT_DOUBLE;
    switch( kind )
    {
      case tNEQ: // +-1 -- already ok
	result.push_back((fst == VT_INT)? BC_ICMP : BC_DCMP );
	break;
      case tEQ:
	if ( wides ) {
	  result.push_back( BC_DCMP );
	  result.push_back( BC_ILOAD0 );
	}
	result.push_back( BC_IFICMPE );
	result.push_back( 6 );
	result.push_back( 0 );
	result.push_back( BC_ILOAD0 );
	result.push_back( BC_JA );
	result.push_back( 3 );
	result.push_back( 0 );
	result.push_back( BC_ILOAD1 );
	
	break;
      case tGT: if ( wides ) {
	result.push_back( BC_DCMP );
	result.push_back( BC_ILOAD0 ); 
	result.push_back( BC_IFICMPG );
      }
      else result.push_back( BC_IFICMPG );
      result.push_back( 6 );
      result.push_back( 0 );
      result.push_back( BC_ILOAD0 );
      result.push_back( BC_JA );
      result.push_back( 3 );
      result.push_back( 0 );
      result.push_back( BC_ILOAD1 ); 
      break;
      case tLT: if ( wides ) {
	result.push_back( BC_DCMP );
	result.push_back( BC_ILOAD0 );
	result.push_back( BC_IFICMPL );
      }
      else result.push_back( BC_IFICMPL );
      result.push_back( 6 );
      result.push_back( 0 );
      result.push_back( BC_ILOAD0 );
      result.push_back( BC_JA );
      result.push_back( 3 );
      result.push_back( 0 );
      result.push_back( BC_ILOAD1 ); 
      break;
      case tGE: if ( wides ) {
	result.push_back( BC_DCMP );
	result.push_back( BC_ILOAD0 );
	result.push_back( BC_IFICMPGE );
      }
      else result.push_back( BC_IFICMPGE );
      result.push_back( 6 );
      result.push_back( 0 );
      result.push_back( BC_ILOAD0 );
      result.push_back( BC_JA );
      result.push_back( 3 );
      result.push_back( 0 );
      result.push_back( BC_ILOAD1 ); 
      break;
      case tLE: if ( wides ) {
	result.push_back( BC_DCMP );
	result.push_back( BC_ILOAD0 );
	result.push_back( BC_IFICMPLE );
      }
      else result.push_back( BC_IFICMPLE );
      result.push_back( 6 );
      result.push_back( 0 );
      result.push_back( BC_ILOAD0 );
      result.push_back( BC_JA );
      result.push_back( 3 );
      result.push_back( 0 );
      result.push_back( BC_ILOAD1 ); 
      break;
      
      
      default: return false;
    } 
    return true;
    
  }
  static VarType string_to_int( VarType from, std::vector<Instruction>& convs)
  {
    if ( from == VT_STRING ) { convs.push_back( BC_S2I ); return VT_INT; }
    else return from;
  }
  
  bool MVMTranslator::handle_special( TokenKind kind, VarType fst, VarType snd, std::vector<Instruction>& fst_conv, std::vector<Instruction>& snd_conv, std::vector<uint8_t>& result )
  { 
    //imod and bitwise ops
    switch( kind ) 
    {
      case tMOD: result.push_back( BC_IMOD ); break;
      case tAOR: result.push_back( BC_IAOR ); break;
      case tAAND: result.push_back( BC_IAAND ); break;
      case tAXOR: result.push_back( BC_IAXOR ); break;
      default:
	return false;
    }
    add_all( fst_conv, get_conversion_vector( fst, VT_INT ).first ); fst = VT_INT;
    add_all( snd_conv, get_conversion_vector( snd, VT_INT ).first ); snd = VT_INT;
    
    return true;
  } 
  
  void MVMTranslator::visitBinaryOpNode( BinaryOpNode* node )
  {    
    pre_node_actions( node );
    
    std::vector<Instruction> left_conversions;
    std::vector<Instruction> right_conversions;
    
    VarType left_type =  string_to_int( get_type( node->left() ), left_conversions);
    VarType right_type = string_to_int( get_type( node->right() ), right_conversions );
    std::vector<uint8_t> instr;
    
    const TokenKind kind = node->kind();
    bool done = handle_arithmetic( node->kind(), left_type, right_type, left_conversions, right_conversions, instr ) ||
    handle_logic( node-> kind(), left_type, right_type, left_conversions, right_conversions, instr) ||
    handle_booleans( node-> kind(), left_type, right_type, left_conversions, right_conversions, instr) ||
    
    handle_special( node-> kind(), left_type, right_type, left_conversions, right_conversions, instr );
    
    
    if (!done) throw logic_error( std::string("Nothing can beat this token! ") + tokenStr( kind ) ) ;
    
    if (instr.size() == 0) { throw logic_error( std::string( "error processing token: " ) + tokenStr(node->kind()) ); }
    node-> left()-> visit( this );
    _contexts->topmost_function_record()-> embrace( left_conversions );
    node-> right()-> visit( this );
    _contexts->topmost_function_record()-> embrace( right_conversions );
    ins( instr );     
  }
  
  void MVMTranslator::visitReturnNode( ReturnNode* node ) 
  { 
    pre_node_actions( node );
    if (node->returnExpr() != NULL )
    {
      node->returnExpr()->visit( this );
      _contexts->topmost_function_record()-> embrace( get_conversion_vector( get_type( node->returnExpr() ), _contexts->topmost_function_record()-> return_type() ).first ); 
    }
    ins( BC_RETURN );
  }
  
  void MVMTranslator::visitUnaryOpNode( UnaryOpNode* node ) {
    switch (node-> kind()){
      case tSUB:  
	if (node-> operand()-> isIntLiteralNode()) 
	  push_ivalue( - node-> operand()-> asIntLiteralNode()-> literal());
	else if( node->operand()->isDoubleLiteralNode() ) 
	  push_dvalue( - node-> operand()-> asDoubleLiteralNode()-> literal());
	else {
	  node-> operand()-> visit( this );
	  switch ( get_type(node-> operand() ) )
	  {
	    case VT_INT: ins( BC_INEG ); break;
	    case VT_DOUBLE: ins( BC_DNEG ); break;
	    default: throw logic_error( "Can only negate doubles and integers on TOS!" );
	  } 
	};
	break; 
	    case tNOT: 
	      if (node-> operand()-> isIntLiteralNode()) 
		push_ivalue(  node-> operand()-> asIntLiteralNode()-> literal() == 0);
	      else if( node->operand()->isDoubleLiteralNode() ) 
		push_dvalue( !( node-> operand()-> asDoubleLiteralNode()-> literal() == 0.0 ));
	      else {
		node->operand()->visit( this );
		Bytecode& bc = current_function()->code;
		Label tru( &bc ); 
		Label end( &bc );
		switch ( get_type(node-> operand() ) )
		{ 
		  case VT_DOUBLE: 
		    ins( BC_DLOAD0);
		    ins( BC_DCMP ); 
		  case VT_INT:  
		    ins( BC_ILOAD0); 
		    bc.addBranch( BC_IFICMPE, tru);
		    ins( BC_ILOAD0 );
		    bc.addBranch( BC_JA, end );
		    bc.bind( tru );
		    ins( BC_ILOAD1 );
		    bc.bind( end );
		    
		    break; 
		  default: throw logic_error( "Can only logically NOT doubles and integers on TOS!" );
		} 
	      }; break;
	      
		  default: throw logic_error( std::string("Not implemented unary operation ") + tokenStr( node->kind()));
    }
  }
  
  void MVMTranslator::visitIfNode( IfNode* node )
  {
    pre_node_actions( node );
    node-> ifExpr()-> visit( this );
    Bytecode& bc = current_function()->code;
    Label else_label(&bc);
    Label end_label(&bc);
    
    //todo: special case when the condition is a literal (depending on that we can only visit one branch) s AND special case when the condition is a binary node.
    
    ins( BC_ILOAD0 );
    
    if( node->elseBlock() == NULL )
    { 
      bc.addBranch(BC_IFICMPE, end_label);
      node-> thenBlock()-> visit( this );
      bc.bind( end_label );
    }
    else { // has else blocks
      
      bc.addBranch( BC_IFICMPE, else_label );
      node-> thenBlock()-> visit( this );
      bc.addBranch( BC_JA, end_label );
      bc.bind( else_label );
      node-> elseBlock()-> visit( this );
      bc.bind( end_label );    
    }
  }
  
  void MVMTranslator::visitCallNode( CallNode* node ) 
  {
    FunctionRecord* fun = _contexts->find_function( node->name() );
    if ( fun == NULL ) throw logic_error( std::string("Can't call an unknown function ") + node->name() );
    
    for (int32_t i = node->parametersNumber()-1 ; i >=0 ; --i )
    {
      AstNode* param  = node->parameterAt( i);
      param->visit( this );
      perform_conversion( get_type(param), fun->signature[i+1].first );	    
    }
    
    ins16(BC_CALL, fun->id);
  }
  
  
  void MVMTranslator::visitWhileNode( WhileNode* node )
  {
    FunctionRecord* rec = _contexts->topmost_function_record();
    Bytecode& bc = rec->code;
    
    Label cond( &bc );
    Label end( &bc );
    bc.bind( cond );
    node-> whileExpr()-> visit( this );
    ins( BC_ILOAD0 );
    bc.addBranch( BC_IFICMPE, end );
    node-> loopBlock()-> visit( this );
    bc.addBranch( BC_JA, cond );
    bc.bind( end );    
  }
  void MVMTranslator::visitForNode( ForNode* node ) 
  {
    make_context();
    
    FunctionRecord* rec = _contexts->topmost_function_record();
    Bytecode& bc = rec->code;
    
    
    Label cond( &bc );
    Label end( &bc );
    
    const Variable* idx = _contexts->add_var( node-> var() );
    BinaryOpNode* range = node->inExpr()->asBinaryOpNode();
    if ( range == NULL || range->kind() != tRANGE )
      throw logic_error("For node needs a proper range list!");
    
    //init 
    const Variable* to = _contexts-> add_var("$$for_to" , VT_INT );
    range-> left()-> visit( this );
    perform_conversion( get_type( range->left() ), VT_INT );
    store_var( idx );
    
    range-> right()-> visit( this );
    perform_conversion( get_type( range->right() ), VT_INT );
    store_var( to ); 
    
    bc.bind( cond );
    
    load_var( idx );
    load_var( to );
     
    bc.addBranch( BC_IFICMPG, end );
    
    node->body()->visit( this );
    
    load_var( idx );
    ins( BC_ILOAD1 );
    ins( BC_IADD );
    store_var( idx );
    bc.addBranch( BC_JA, cond );
    bc.bind( end );
    destroy_context();
  }
}