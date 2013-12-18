//
//  AstInfoExtractor.h
//  VM_3
//
//  Created by Hatless Fox on 12/12/13.
//  Copyright (c) 2013 WooHoo. All rights reserved.
//

#ifndef VM_3_AstInfoExtractor_h
#define VM_3_AstInfoExtractor_h

#include <vector>
#include <set>

#include "ast.h"
#include "visitors.h"

#include "AstToMCConverter.h"

using namespace mathvm;

// Registers all functions, perferms post-order traversals
// Marks all 'global' valiables (vars that used in inner functions)
class AstInfoExtractor: public AstBaseVisitor {
  
public: //methods
  std::vector<AstFunction *> functions() { return m_functions; }
  
  AstInfoExtractor(AstToMCConverter *conv):m_mc_conv(conv), m_scope_cnt(0) {}
  
  void handle_function_definition(AstFunction *func) {
    GeneratedMCF *fn =  m_mc_conv->registerFunction(func);
    m_scope_stack.push(m_scope_cnt++);
    m_globals.push_back(0);
    m_fn_id_stack.push(fn->id());
    
    //std::cout << "Fn " << func->name() << " has id " << fn->id() << std::endl;
    
    //init infos of AstVars that corresponds to parameters
    Scope *scope = func->node()->body()->scope();
    for (uint32_t i = 0; i < func->parametersNumber(); ++i) {
      AstVar * param = scope->lookupVariable(func->parameterName(i));
      param->setInfo(new FirstPassVarInfo(m_fn_id_stack.top(), m_scope_stack.top(), false));
    }
    
    func->node()->visit(this);

    m_fn_id_stack.pop();
    m_scope_stack.pop();
    fn->set_globals(m_globals.at(fn->id()));
    m_functions.push_back(func);
  }

  
private: //methods
  
  void visitBlockNode(BlockNode* node) {
    assert(!m_scope_stack.empty());
    
    Scope::VarIterator vi(node->scope());
    while (vi.hasNext()) {
      vi.next()->setInfo(new FPVI(m_fn_id_stack.top(), m_scope_stack.top(), false));
    }

    //emulate forward declaration
    Scope::FunctionIterator fi(node->scope());
    while (fi.hasNext()) { handle_function_definition(fi.next()); }
    
    AstBaseVisitor::visitBlockNode(node);
  }
  
  void visitLoadNode(LoadNode* node) {
    FPVI *v_i = (FPVI *)node->var()->info();
    assert(v_i);
    updateVarGlobalState(node->var());
  }

  void visitStoreNode(StoreNode* node) {
    node->value()->visit(this);
    updateVarGlobalState(node->var());
  }

  
private: //fields
  
  void updateVarGlobalState(AstVar const * var) {
    FPVI *v_i = (FPVI *)var->info();
    if (v_i->scope_id != m_scope_stack.top() && !v_i->global) {
      //new global found
      v_i->global = true;
      v_i->globals_ind = ++m_globals.at(v_i->func_id);
      //std::cout << "Global var " << var->name() << "(";
      //std::cout << var << ")"<< " with id ";
      //std::cout << v_i->func_id << ":" << v_i->globals_ind << std::endl;
    }
  }
  
  std::vector<AstFunction *> m_functions;
  AstToMCConverter *m_mc_conv;
  
  uint64_t m_scope_cnt;
  std::stack<uint64_t> m_scope_stack;
  std::stack<uint16_t> m_fn_id_stack;
  std::vector<uint64_t> m_globals;
};


#endif
