//
//  AstToBCConverter.h
//  VM_2
//
//  This class "knows" how to traverse Ast, what general view of generated code,
//  maintains some auxilary data besides code
//
//
//  Created by Hatless Fox on 10/25/13.
//  Copyright (c) 2013 WooHoo. All rights reserved.
//

#ifndef __VM_2__AstToBCConverter__
#define __VM_2__AstToBCConverter__

#include <iostream>
#include <stack>
#include <vector>
#include <map>

#include "ast.h"
#include "visitors.h"
#include "FenrirInterpreter.h"
#include "StackIsaGenerator.h"

using namespace mathvm;

class AstToBCTranslator: public AstBaseVisitor {

public: //methods
  Status *convert(AstFunction *main) {
    handle_function_definition(main);
  
    m_code->prepareForExec(m_scopes.size());
    return NULL; // TODO: impl proper status
  }
  
  AstToBCTranslator():m_code(new FenrirInterpreter()), m_active_assigments(0) {}
  FenrirInterpreter *code() { return m_code; }
  
  
  
private: //methods
#define VISITOR_FUNCTION(type, name) virtual void visit##type(type* node);
  FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION
  void generateTrueTest(Label *label);
  void handle_function_definition(AstFunction *func);
  Bytecode* curr_bc();
  inline VarType tos_type() {
    return m_tos_types.size() ? m_tos_types.top() : VT_INVALID;
  }
  
  inline void registerFunction(uint64_t func_key, uint16_t func_id) {
    m_funcAddr2Id[func_key] = func_id;
  }
  
  inline uint16_t getRegisteredFuncId(uint64_t func_key) {
    return m_funcAddr2Id[func_key];
  }
  
  
private: //fields
  FenrirInterpreter *m_code;
  StackIsaGenerator m_isa;
  unsigned m_active_assigments;
  
  std::stack<VarType> m_tos_types;
  std::stack<BytecodeFunction *> m_curr_funcs;
  std::vector<Scope *> m_scopes;
  
  std::map<uint64_t, uint16_t> m_funcAddr2Id;
};

#endif /* defined(__VM_2__AstToBCConverter__) */
