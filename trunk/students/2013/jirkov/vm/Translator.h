
#pragma once

#include <string.h> 
#include <fcntl.h>
#include <sys/stat.h> 
#include <iostream>
#include <list>
#include "helpers.h"
#include <stdexcept>

#include "../../../../include/ast.h"
#include "../../../../include/mathvm.h"
#include "../../../../include/visitors.h"
#include "../../../../vm/parser.h" 


#include "AbstractContext.h"
#include "FunctionRecord.h"
#include "TranslatorConstantPool.h"

#include "AbstractInterpreter.h"


namespace mathvm {
  
  class StaticContext;
  class FunctionContext;
  
  class  MVMTranslator : public AbstractInterpreter   {
  
  private:      
    FunctionNode* const _root;
    void pre_node_actions( AstNode* node );
  public: 
    
    TranslatorConstantPool constant_pool; 
     
    MVMTranslator( InterpreterCodeImpl * code, FunctionNode * root );
    
    MVMTranslator( MVMTranslator * parent, FunctionNode * root );
    
    
     
    
    void run();
    
    /*
     * helpers
     */ 
    void ins( const Instruction i ) { _contexts->topmost_function_record()-> code.addByte( i ); }
    void ins( const Instruction i, const int16_t p1, const int16_t p2 ) { ins(i); ins16(p1); ins16(p2); }
    void ins16( const Instruction i, const int16_t p1 ) { ins(i); ins16(p1); }
    void ins16( const Instruction i, const int16_t p1, const int16_t p2 ) { ins(i); ins16(p1); ins16(p2); }
    void ins16( const int16_t i ) { _contexts->topmost_function_record()->code.addInt16( i ); }
    void ins64( const int64_t i ) { _contexts->topmost_function_record()->code.addInt64( i ); }
    void ins64( const Instruction i, const int64_t p1 ) { ins(i); _contexts->topmost_function_record()->code.addInt64( p1 ); }
    void ins64( const Instruction i, const double p1 ) { ins(i); _contexts->topmost_function_record()->code.addDouble( p1 ); }
    void ins( std::vector<uint8_t> const& vector ) {
      for (std::vector<uint8_t>::const_iterator it = vector.begin(); it != vector.end(); ++it )
	_contexts->topmost_function_record()->code.addByte( *it );     
    }
    
    void push_ivalue( const int64_t value );
    void push_dvalue( const double value );
    void load_var ( const Variable *const var );
    void store_var( const Variable *const var ); 
    FunctionRecord* current_function() { return (_contexts == NULL)? NULL : _contexts->topmost_function_record(); }
    
    
    /*
     * Processors for different node types
     */
    
    virtual void visitBlockNode( BlockNode* node );
    
    virtual void visitDoubleLiteralNode( DoubleLiteralNode* node );
    virtual void visitIntLiteralNode( IntLiteralNode* node );
    virtual void visitStringLiteralNode( StringLiteralNode* node );
    
    virtual void visitFunctionNode( FunctionNode* node );
    virtual void visitPrintNode( PrintNode * node );
    
    virtual void visitStoreNode( StoreNode* node );
    virtual void visitLoadNode( LoadNode* node );
    
    virtual void visitReturnNode( ReturnNode* node );
    virtual void visitBinaryOpNode( BinaryOpNode* node );
    virtual void visitUnaryOpNode( UnaryOpNode* node );
  
    virtual void visitIfNode( IfNode* node ); 
  
    virtual void visitCallNode( CallNode* node ); 
    
    virtual void visitWhileNode( WhileNode* node );
    virtual void visitForNode( ForNode* node );
    /*    
     *            DO(ForNode, "for")                  \  
     *            DO(WhileNode, "while")              \  
     *            
     *            DO(NativeCallNode, "native call")   \   
     */
    
  bool handle_arithmetic( TokenKind kind, VarType fst, VarType snd, std::vector<Instruction>& fst_conv, std::vector<Instruction>& snd_conv, std::vector<uint8_t>& result );
  bool handle_booleans( TokenKind kind, VarType fst, VarType snd, std::vector<Instruction>& fst_conv, std::vector<Instruction>& snd_conv, std::vector<uint8_t>& result );
  bool handle_logic( TokenKind kind, VarType fst, VarType snd, std::vector<Instruction>& fst_conv, std::vector<Instruction>& snd_conv, std::vector<uint8_t>& result );
  bool handle_special( TokenKind kind, VarType fst, VarType snd, std::vector<Instruction>& fst_conv, std::vector<Instruction>& snd_conv, std::vector<uint8_t>& result );
     
  virtual ~MVMTranslator() {}
    
  public:
       
    static std::pair<std::vector<Instruction>, VarType> get_conversion_vector( VarType from, VarType to );
    void perform_conversion( VarType from, VarType to );
  };
  
}



