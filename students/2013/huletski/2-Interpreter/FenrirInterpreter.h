//
//  FenrirCode.h
//  VM_2
//
//  Created by Hatless Fox on 10/25/13.
//  Copyright (c) 2013 WooHoo. All rights reserved.
//

#ifndef __VM_2__FenrirCode__
#define __VM_2__FenrirCode__

#include <stack>
#include <vector>
#include "mathvm.h"
#include "OperandStack.h"

using namespace mathvm;

class FenrirInterpreter: public Code {
public:
  FenrirInterpreter():m_vector_cache_ind(-1) {}

  virtual Status* execute(std::vector<Var*>& vars);
#define OP_FUNCTION(op_name, op) \
  void exec##op_name##uint64_t() { \
    int64_t val1 = m_op_stack.pop();      \
    int64_t val2 = m_op_stack.pop();      \
    m_op_stack.push(val1 op val2);     \
  } \

#define OP_FUNCTION_DBL(op_name, op) \
  void exec##op_name##double() { \
  double val1 = i2d(m_op_stack.pop());      \
  double val2 = i2d(m_op_stack.pop());      \
  m_op_stack.push(d2i(val1 op val2));     \
}                                    \

  OP_FUNCTION_DBL(Add, +)
  OP_FUNCTION(Add, +)
  OP_FUNCTION_DBL(Sub, -)
  OP_FUNCTION(Sub, -)
  OP_FUNCTION_DBL(Mul, *)
  OP_FUNCTION(Mul, *)
  OP_FUNCTION_DBL(Div, /)
  OP_FUNCTION(Div, /)
  OP_FUNCTION(Mod, %)
  
  OP_FUNCTION(Aor, |)
  OP_FUNCTION(Aand, &)
  OP_FUNCTION(Axor, ^)
  
#undef OP_FUNCTION

  void prepareForExec(size_t func_cnt) {
    m_func_intimates.resize(func_cnt);
  }

  inline void loadVar(int16_t ind) { m_op_stack.push(locals->operator[](ind)); }
  inline void storeVar(int16_t ind){locals->operator[](ind) = m_op_stack.pop();}
                    
                    
private:
  
  inline int64_t d2i(double val) {
    int64_t *ptr = (int64_t *)&val;
    return *ptr;
  }

  inline double i2d(int64_t val) {
    double *ptr = (double *)&val;
    return *ptr;
  }

  
  
  void functionCallPrologue(BytecodeFunction *fn);
  void functionCallEpilogue();
  void exec_function();
    
  
  inline int64_t getVarFromCtx(uint16_t ctx, uint16_t local) {
    return getVectorFromCache(m_func_intimates[ctx].top())[local];
  }
  
  
  inline void setVarToCtx(uint16_t ctx, uint16_t local, int64_t val) {
    getVectorFromCache(m_func_intimates[ctx].top())[local] = val;
  }
  
  inline std::vector<int64_t> & getVectorFromCache(int64_t desc) {
    return m_vector_cache[desc];
  }
  
  inline int64_t allocVectorFromCache(uint32_t sz) {
    if (m_vector_cache.size() <= ++m_vector_cache_ind) {
      m_vector_cache.push_back(std::vector<int64_t>(fn->localsNumber()));
    }
    m_vector_cache[m_vector_cache_ind].resize(sz);
    return m_vector_cache_ind;
  }

  inline void returnLastVectorToCache() {
    m_vector_cache_ind--;
  }


  
private:
  OperandStack m_op_stack;
  stack<std::pair<int32_t, BytecodeFunction *> > m_call_stack;
  
  std::vector<std::stack<int64_t> > m_func_intimates;
  
  std::vector<std::vector<int64_t> > m_vector_cache;
  int64_t m_vector_cache_ind;
  
  BytecodeFunction *fn;
  uint32_t ic;
  Bytecode *bc;
  std::vector<int64_t> *locals;
};

#endif /* defined(__VM_2__FenrirCode__) */
