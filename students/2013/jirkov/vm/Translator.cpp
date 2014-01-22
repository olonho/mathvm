#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <list>
#include <memory>
#include <stdexcept> 
#include "Translator.h"
#include "Variable.h"
#include "TypeInferrer.h"

namespace mathvm {
  
  void eat_visitor( MVMTranslator& translator );
  
  Status* BytecodeTranslatorImpl::translateBytecode( string const & program, InterpreterCodeImpl ** code ) 
  {
    Parser parser;
    Status * status = parser.parseProgram(program);
    if ( status && status->isError() ) return status;
    
    AstFunction* root = parser.top();
    
    Infer inferrer( root-> node() );
    inferrer.visitFunctionNode( root->node() );
    MVMTranslator visitor( (InterpreterCodeImpl*) NULL, root->node() );
    try { visitor.run(); }
    catch( logic_error & e) 
    {
      //visitor._result->code.dump( std::cout );
      return new Status(e.what());
    }
    eat_visitor( visitor );
    
    Cleaner( root-> node() );
    
    return new Status();  
  }
  
  Status * BytecodeTranslatorImpl::translate( std::string const & program, Code ** code ) 
  {
    return translateBytecode(program, (InterpreterCodeImpl**)code);
  }
  
  
  MVMTranslator::MVMTranslator( InterpreterCodeImpl * code, FunctionNode * root ) 
  : _root( root ) {}
  
  void MVMTranslator::run()
  {
    visitFunctionNode( _root );
    /*
    for( std::vector<FunctionRecord*>::iterator it = _functions.begin(); it != _functions.end(); ++it )
      std::cout<< (*it)->name << " , id = " << (*it)->id << std::endl;
      */
   
  }
  
  void MVMTranslator::load_var( const Variable *const var )
  {
    if ( var->fun_id == current_context_id() ) switch( var-> type ){
      case VT_INT:  
	switch ( var-> idx ) {
	  case 0: ins( BC_LOADIVAR0 ); break;
	  case 1: ins( BC_LOADIVAR1 ); break;
	  case 2: ins( BC_LOADIVAR2 ); break;
	  case 3: ins( BC_LOADIVAR3 ); break;
	  default: ins16( BC_LOADIVAR, var-> idx );
	}; break; 
	  case VT_DOUBLE: 
	    switch ( var-> idx ) {
	      case 0: ins( BC_LOADDVAR0 ); break;
	      case 1: ins( BC_LOADDVAR1 ); break;
	      case 2: ins( BC_LOADDVAR2 ); break;
	      case 3: ins( BC_LOADDVAR3 ); break;
	      default: ins16( BC_LOADDVAR, var-> idx );
	    }; break;
	    
	      case VT_STRING: 
		switch ( var-> idx ) {
		  case 0: ins( BC_LOADSVAR0 ); break;
		  case 1: ins( BC_LOADSVAR1 ); break;
		  case 2: ins( BC_LOADSVAR2 ); break;
		  case 3: ins( BC_LOADSVAR3 ); break;
		  default: ins16( BC_LOADSVAR, var-> idx );
		}; break;
		  default:
		    throw logic_error( "Unsupported store var type" );
    } 
    else switch( var-> type ){
		  case VT_INT:    ins16( BC_LOADCTXIVAR, var-> fun_id, var->idx ); break;
		  case VT_DOUBLE: ins16( BC_LOADCTXDVAR, var-> fun_id, var->idx ); break;
		  case VT_STRING: ins16( BC_LOADCTXSVAR, var-> fun_id, var->idx ); break;
		  default: throw logic_error( "Unsupported store var type" );
    }
  } 
  
  void MVMTranslator::store_var( const Variable *const var )
  { 
    if ( var->fun_id == current_context_id() ) switch( var-> type ) {
		  case VT_INT:  
		    switch ( var-> idx ) {
		      case 0: ins( BC_STOREIVAR0 ); break;
		      case 1: ins( BC_STOREIVAR1 ); break;
		      case 2: ins( BC_STOREIVAR2 ); break;
		      case 3: ins( BC_STOREIVAR3 ); break;
		      default: ins16( BC_STOREIVAR, var-> idx );
		    }; break; 
		  case VT_DOUBLE: 
		    switch ( var-> idx ) {
		      case 0: ins( BC_STOREDVAR0 ); break;
		      case 1: ins( BC_STOREDVAR1 ); break;
		      case 2: ins( BC_STOREDVAR2 ); break;
		      case 3: ins( BC_STOREDVAR3 ); break;
		      default: ins16( BC_STOREDVAR, var-> idx );
		    }; break;
		    
		  case VT_STRING: 
		    switch ( var-> idx ) {
		      case 0: ins( BC_STORESVAR0 ); break;
		      case 1: ins( BC_STORESVAR1 ); break;
		      case 2: ins( BC_STORESVAR2 ); break;
		      case 3: ins( BC_STORESVAR3 ); break;
		      default: ins16( BC_STORESVAR, var-> idx );
		    }; break;
		  default:
		    throw logic_error( "Unsupported store var type" );
    } 
    else switch( var-> type ) {
			      case VT_INT:    ins16( BC_STORECTXIVAR, var-> fun_id, var->idx ); break;
			      case VT_DOUBLE: ins16( BC_STORECTXDVAR, var-> fun_id, var->idx ); break;
			      case VT_STRING: ins16( BC_STORECTXSVAR, var-> fun_id, var->idx ); break;
			      default: throw logic_error( "Unsupported store var type" );
    }
  }
  
  
  std::pair<std::vector<Instruction>, VarType> MVMTranslator::get_conversion_vector( VarType from, VarType to )
  {
    std::vector<Instruction> res;
    VarType type = from;
    switch( from ) 
    {
      case VT_INT: switch( to ) 
      {
	case VT_INT: break;
	case VT_DOUBLE: res.push_back( BC_I2D );  type = VT_DOUBLE; break;
	case VT_STRING: throw logic_error( "Can't convert from int to string!" );     
	default: throw logic_error( "Invalid conversion destination" );
      }; break;
      case VT_DOUBLE: switch( to )
      {
	case VT_INT: res.push_back( BC_D2I ); type = VT_INT; break;
	case VT_DOUBLE: break;
	case VT_STRING: throw logic_error( "Can't convert from double to string!" );     
	default: throw logic_error( "Invalid conversion destination" );
      }; break;
      case VT_STRING: switch( to )
      {
	case VT_INT: res.push_back( BC_S2I ); break;
	case VT_DOUBLE: res.push_back( BC_S2I ); res.push_back( BC_I2D ); type = VT_DOUBLE; break;
	case VT_STRING: break;
	default: throw logic_error( "Invalid conversion destination" );
      }; break;
      default: throw logic_error( "Invalid type can't be converted to anything!" );
    }
    return make_pair( res, type );
  } 
  
  void MVMTranslator::perform_conversion(VarType from, VarType to)
  {
    std::pair<std::vector<Instruction>, VarType> convs = get_conversion_vector( from, to );
    if (convs.second != to ) throw logic_error(std::string("Conversion error: can't convert ") + typeToName( from ) + " to " + typeToName( to ) );
    else _contexts->topmost_function_record()->embrace( convs.first );
  }
  
  
}
