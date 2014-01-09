//
//  AstToMCConverter.h
//  VM_3
//
//  Created by Hatless Fox on 12/11/13.
//  Copyright (c) 2013 WooHoo. All rights reserved.
//

#ifndef __VM_3__AstToMCConverter__
#define __VM_3__AstToMCConverter__

#include <iostream>
#include <stack>
#include <vector>
#include <map>

#include "ast.h"
#include "visitors.h"
#include "MachCodeContainer.h"
#include "RegistryISAGenerator.h"

using namespace mathvm;

class AstToMCConverter: public AstBaseVisitor {
  
public: //methods
  Status *convert(std::vector<AstFunction *> inner_funcs, uint64_t fn_cnt) {
    m_err_status = NULL;
    
    m_isa.setFuncsCnt(fn_cnt);
    
    while (!inner_funcs.empty()) {
      AstFunction *f = inner_funcs.back();
      inner_funcs.pop_back();
      
      handle_function_definition(f);
    }
    
    m_code->set_asm_code(m_isa.generateAsm());
    return m_err_status; // TODO: impl proper status
  }

  AstToMCConverter():m_code(new MachCodeContainer()), m_active_assigments(0), m_err_status(NULL) {}
  MachCodeContainer *code() { return m_code; }
  inline Status* errorStatus() { return m_err_status; }
  
  
  inline GeneratedMCF * registerFunction(AstFunction* function) {
    BytecodeFunction *bc_fn = new BytecodeFunction(function);
    GeneratedMCF *fn = new GeneratedMCF(bc_fn);
    m_code->addFunction(fn);
    
    if (isFunctionNative(function)) {
      const char * native_name = function->node()->body()->nodeAt(0)->asNativeCallNode()->nativeName().c_str();
      void *code = dlsym(RTLD_DEFAULT, native_name);
      if (!code) {
        logError("Unable to locate code for native function " + std::string(native_name));
      }
      fn->set_code(code);
    }
    return fn;
  }
  
  RegistryISAGenerator & isa() { return m_isa; }
  
private: //methods
#define VISITOR_FUNCTION(type, name) virtual void visit##type(type* node);
  FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION
  void generateTrueTest(mathvm::Label *label);
  void handle_function_definition(AstFunction *func);
  
  inline AAVI * cloneAAVI(const AstVar * var) {
    return ((AAVI *)var->info())->clone();
  }
  
  inline EvalResult recent_eval_result() {
    if (m_eval_results.empty()) {
      assert("Try to get non existent eval result");
    }
    EvalResult er = m_eval_results.top();
    m_eval_results.pop();
    return er;
  }

  inline void logError(std::string error) {
    if (m_err_status) {  return; }
    m_err_status = new Status(error.c_str());
  }
  
  inline bool isFunctionNative(AstFunction *func) {
    return func->node()->body() && func->node()->body()->nodes() &&
    func->node()->body()->nodeAt(0)->isNativeCallNode();
  }
  
  void addAsmInfo(AstVar *var);
  
private: //fields
  MachCodeContainer *m_code;
  RegistryISAGenerator m_isa;
  unsigned m_active_assigments;
  Status *m_err_status;
  GeneratedMCF *m_curr_fn;
  
  std::stack<EvalResult> m_eval_results;
};


#endif /* defined(__VM_3__AstToMCConverter__) */
