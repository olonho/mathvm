//
//  MachCodeContainer.h
//  VM_3
//
//  Created by Hatless Fox on 12/11/13.
//  Copyright (c) 2013 WooHoo. All rights reserved.
//

#ifndef __VM_3__MachCodeContainer__
#define __VM_3__MachCodeContainer__

#include <iostream>

#include "RegistryISAGenerator.h"
#include "mathvm.h"
#include "ast.h"

#ifdef XCODE_BUILD
#include "AsmJit.h"
#else
#include <AsmJit/AsmJit.h>
#endif

using namespace mathvm;
using namespace AsmJit;


#define PROPERTY(type, name) \
  void set_##name(type name) { m_##name = name; } \
  type name() { return m_##name; }


class MachCodeContainer : public Code {
public:
  PROPERTY(void *, asm_code);
  
  virtual ~MachCodeContainer() {}
  virtual Status* execute(vector<Var*>& vars) {
    void (*program)() = function_cast<void (*)(void)>(m_asm_code);
    assert(program && "Unable to get compiled program");
    program();
    //TODO: make status descriptive
    return NULL;
  }
private:
  void * m_asm_code;
};

class GeneratedMCF : public TranslatedFunction {
public:
  GeneratedMCF(BytecodeFunction* bc_func):
    TranslatedFunction(bc_func->name(), bc_func->signature()) {
    m_code = NULL;
    m_ajFunc = 0;
  }

  void initECall(ECall *call) {
    if (m_ajFunc)    { call->getTarget() = m_ajFunc->getEntryLabel(); }
    else if (m_code) { call->getTarget() = imm((sysint_t)m_code);     }
    else             { m_postponed_calls.push_back(call);             }
  }
  
  void generateAjFunc(RegistryISAGenerator *isa) {
    if (m_code) { return; }
    m_ajFunc = isa->newFunction(signature());
    for (size_t i = 0; i < m_postponed_calls.size(); ++i) {
      initECall(m_postponed_calls[i]);
    }
  }
  
  VarType returnType() { return signature()[0].first; }
  virtual ~GeneratedMCF() {}
  
  virtual Status* execute(vector<Var*>* vars = 0) {assert(0 && "Not implemented");}
  virtual void disassemble(ostream& out) const { assert(0 && "Not implemented");}
  
  EFunction * asmjitFunction() { return m_ajFunc; }
  
  PROPERTY(void *, code)
  PROPERTY(uint64_t, globals)
private:
  EFunction *m_ajFunc;
  
  std::vector<ECall *> m_postponed_calls;
  void *m_code;
  uint64_t m_globals;
};

#endif /* defined(__VM_3__MachCodeContainer__) */
