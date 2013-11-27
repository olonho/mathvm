//
//  FenrirCode.cpp
//  VM_2
//
//  Created by Hatless Fox on 10/25/13.
//  Copyright (c) 2013 WooHoo. All rights reserved.
//

#include "FenrirInterpreter.h"

#include <vector>
#include <utility>
#include <cfloat>
#include <cmath>

#include "ast.h"
#include "mathvm.h"

#include <AsmJit/AsmJit.h>
using namespace AsmJit;

Status* FenrirInterpreter::execute(vector<Var*>& vars) {
  BytecodeFunction *main = (BytecodeFunction *)functionByName(AstFunction::top_name);
  
  ic = 0;
  fn = main;
  
  functionCallPrologue(main);
  exec_function();
  
  return NULL;
}

void FenrirInterpreter::functionCallPrologue(BytecodeFunction *new_fn) {
  m_call_stack.push(std::make_pair(ic, fn));
  
  fn = new_fn;
  bc = fn->bytecode();
  ic = 0;
  
  m_func_intimates[fn->scopeId()].push(allocVectorFromCache(fn->localsNumber()));
  
  m_locals = &getVectorFromCache(m_func_intimates[fn->scopeId()].top());
  
  //init params
  for(int32_t param = fn->parametersNumber(); param > 0; --param) {
    setVarToCtx(fn->scopeId(), param, m_op_stack.pop());
  }
}

void FenrirInterpreter::functionCallEpilogue() {
  returnLastVectorToCache();
  m_func_intimates[fn->scopeId()].pop();
  
  std::pair<uint32_t, BytecodeFunction *> stacked = m_call_stack.top();
  m_call_stack.pop();
  
  fn = stacked.second;
  ic = stacked.first;
  bc = fn->bytecode();
  m_locals = &getVectorFromCache(m_func_intimates[fn->scopeId()].top());
}


void FenrirInterpreter::exec_function() {
  while (ic < bc->length()) {
    uint8_t insn = bc->get(ic);
    ic += 1;
    switch (insn) {
      case BC_DLOAD: {
        m_op_stack.push(d2i(bc->getDouble(ic)));
        ic += 8;
        break;
      }
      case BC_ILOAD:
        m_op_stack.push(bc->getInt64(ic));
        ic += 8;
        break;
      case BC_DLOAD0: m_op_stack.push(d2i(0)); break;
      case BC_ILOAD0: m_op_stack.push(0); break;
      case BC_DLOAD1:  m_op_stack.push(d2i(1)); break;
      case BC_ILOAD1: m_op_stack.push(1); break;
      case BC_DLOADM1: m_op_stack.push(d2i(-1)); break;
      case BC_ILOADM1: m_op_stack.push(-1); break;
      case BC_DADD: execAdddouble(); break;
      case BC_IADD: execAdduint64_t(); break;
      case BC_DSUB: execSubdouble(); break;
      case BC_ISUB: execSubuint64_t(); break;
      case BC_DDIV: execDivdouble(); break;
      case BC_IDIV: execDivuint64_t(); break;
      case BC_DMUL: execMuldouble(); break;
      case BC_IMUL: execMuluint64_t(); break;
      case BC_IMOD: execModuint64_t(); break;
      case BC_IAAND: execAanduint64_t(); break;
      case BC_IAOR: execAoruint64_t(); break;
      case BC_IAXOR: execAxoruint64_t(); break;
      case BC_DNEG: {
        m_op_stack.push(d2i(-i2d(m_op_stack.pop())));
        break;
      }
      case BC_INEG: {
        m_op_stack.push(-m_op_stack.pop());
        break;
      }
      case BC_DPRINT:
        std::cout << i2d(m_op_stack.pop());
        break;
      case BC_IPRINT:
        std::cout << (int64_t) m_op_stack.pop();
        break;
      case BC_SWAP: {
        int64_t val1 = m_op_stack.pop();
        int64_t val2 = m_op_stack.pop();

        m_op_stack.push(val1);
        m_op_stack.push(val2);
        break;
      }
      case BC_LOADDVAR0: case BC_LOADIVAR0: case BC_LOADSVAR0: loadVar(0); break;
      case BC_LOADDVAR1: case BC_LOADIVAR1: case BC_LOADSVAR1: loadVar(1); break;
      case BC_LOADDVAR2: case BC_LOADIVAR2: case BC_LOADSVAR2: loadVar(2); break;
      case BC_LOADDVAR3: case BC_LOADIVAR3: case BC_LOADSVAR3: loadVar(3); break;

      case BC_STOREDVAR0: case BC_STOREIVAR0: case BC_STORESVAR0: storeVar(0);break;
      case BC_STOREDVAR1: case BC_STOREIVAR1: case BC_STORESVAR1: storeVar(1);break;
      case BC_STOREDVAR2: case BC_STOREIVAR2: case BC_STORESVAR2: storeVar(2);break;
      case BC_STOREDVAR3: case BC_STOREIVAR3: case BC_STORESVAR3: storeVar(3);break;
     
      case BC_LOADDVAR: case BC_LOADIVAR: case BC_LOADSVAR:
        loadVar(bc->getInt16(ic));
        ic += 2;
        break;
      case BC_STOREDVAR: case BC_STOREIVAR: case BC_STORESVAR:
        storeVar(bc->getInt16(ic));
        ic += 2;
        break;
        
      case BC_JA:
        ic += bc->getInt16(ic);
        break;
      case BC_IFICMPE: {
        int64_t val1 = m_op_stack.pop();
        int64_t val2 = m_op_stack.pop();
        if (val1 == val2) { ic += bc->getInt16(ic); }
        else { ic += 2; }
        break;
      }
      case BC_IFICMPNE: {
        int64_t val1 = m_op_stack.pop();
        int64_t val2 = m_op_stack.pop();
        if (val1 != val2) { ic += bc->getInt16(ic); }
        else { ic += 2; }
        break;
      }
      
      case BC_DCMP: {
        double val1 = i2d(m_op_stack.pop());
        double val2 = i2d(m_op_stack.pop());

        m_op_stack.push(abs(val1 - val2) < DBL_EPSILON ? 0 : val1 > val2 ? 1 : -1);
        break;
      }
      case BC_ICMP: {
        int64_t val1 = m_op_stack.pop();
        int64_t val2 = m_op_stack.pop();
        m_op_stack.push(val1 == val2 ? 0 : val1 > val2 ? 1 : -1);
        break;
      }
      case BC_SLOAD: {
        m_op_stack.push(bc->getInt16(ic));
        ic += 2;
        break;
      }
      case BC_SPRINT: {
        std::cout << strFromDesc(m_op_stack.pop());
        break;
      }

      case BC_CALL: {
        uint16_t fn_id = bc->getInt16(ic);
        ic += 2;
        
        functionCallPrologue((BytecodeFunction *)functionById(fn_id));
        break;
      }
      case BC_RETURN: {
        functionCallEpilogue();
        break;
      }
      case BC_LOADCTXDVAR: case BC_LOADCTXIVAR: case BC_LOADCTXSVAR: {
        uint16_t ctx_id = bc->getInt16(ic); ic += 2;
        uint16_t local_id = bc->getInt16(ic); ic += 2;
        m_op_stack.push(getVarFromCtx(ctx_id, local_id));
        break;
      }
      case BC_STORECTXIVAR: case BC_STORECTXDVAR: case BC_STORECTXSVAR: {
        uint16_t ctx_id = bc->getInt16(ic); ic += 2;
        uint16_t local_id = bc->getInt16(ic); ic += 2;
        setVarToCtx(ctx_id, local_id, m_op_stack.pop());
        break;
      }
      case BC_D2I: {
        // cast to double bitwise that to int by value
        m_op_stack.push(i2d(m_op_stack.pop()));
        break;
      }
      case BC_I2D: {
        // cast to double by value, that to int bitwise
        m_op_stack.push(d2i(m_op_stack.pop()));
        break;
      }
      case BC_S2I: {
        //Not sure about semantic. Use C-style
        //uint16_t str_id = (uint16_t)m_op_stack.pop();
        //m_op_stack.push(atoi(constantById(str_id).c_str()));
        break;
      }
      case BC_CALLNATIVE: {
        NativeCallExecutor executor(this, bc->getInt16(ic));
        executor.performNativeCall();
        ic += 2;
      }
      default:
        break;
    }

  }

}

FenrirInterpreter::NativeCallExecutor::NativeCallExecutor(FenrirInterpreter *inter, uint16_t nid) {
  m_inter = inter;
  
  std::string const *name;
  Signature const *signature;
  m_code_ref = (void *)m_inter->nativeById(nid, &signature, &name);

  //reserve size that enought to store all args to prevent vector relocation
  m_copied_consts = std::vector<std::string>(signature->size());
  m_signature = *signature;
}

FenrirInterpreter::NativeCallExecutor::~NativeCallExecutor() {
  for (size_t i = 0; i < m_arg_vars.size(); ++i) {
    delete m_arg_vars[i];
  }
}

void FenrirInterpreter::NativeCallExecutor::prepareReturnValue() {
  //setup return value
  switch (returnType()) {
    case VT_DOUBLE: {

      m_compiler.newFunction(CALL_CONV_DEFAULT, FunctionBuilder0<double>());
      break;
    }
    case VT_INT: {
      m_native_fun_bldr.setReturnValue<int64_t>();
      m_compiler.newFunction(CALL_CONV_DEFAULT, FunctionBuilder0<int64_t>());
      break;
    }
    case VT_STRING: {
      m_native_fun_bldr.setReturnValue<char *>();
      m_compiler.newFunction(CALL_CONV_DEFAULT, FunctionBuilder0<char *>());
      break;
    }
    case VT_VOID: {
      m_native_fun_bldr.setReturnValue<void>();
      m_compiler.newFunction(CALL_CONV_DEFAULT, FunctionBuilder0<Void>());
      break;
    }
    default: break;
  }
}

void FenrirInterpreter::NativeCallExecutor::prepareArgs() {
  for (size_t arg_ti = 1; arg_ti < m_signature.size(); ++arg_ti) {
    switch (m_signature.at(arg_ti).first) {
      case VT_DOUBLE: {
        m_native_fun_bldr.addArgument<double>();
        XMMVar *arg = new XMMVar(m_compiler.newXMM(VARIABLE_TYPE_XMM_1D));
        m_arg_vars.push_back(arg);
        
        //aghhrr need to convert to relative address to support x64 (see AssemblerX86X64.c:328)
        // so use workaround for now
        GPVar tmp(m_compiler.newGP());
        m_compiler.mov(tmp, m_inter->locals()->at(arg_ti));
        m_compiler.movq(*arg, tmp);
        m_compiler.unuse(tmp);
        break;
      }
      case VT_INT: {
        m_native_fun_bldr.addArgument<int64_t>();
        GPVar *arg = new GPVar(m_compiler.newGP());
        m_arg_vars.push_back(arg);
        
        m_compiler.mov(*arg, m_inter->locals()->at(arg_ti));
        break;
      }
      case VT_STRING: {
        m_native_fun_bldr.addArgument<char *>();
        uint64_t str_desc = m_inter->locals()->at(arg_ti);
        
        char *data = m_inter->strFromDesc(str_desc);
        if (!m_inter->isStrDescNativePtr(str_desc)) {
          //not native means const, so provide copy to prevent modification
          m_copied_consts.push_back(data);
          data = (char *)m_copied_consts[m_copied_consts.size() - 1].c_str();
        }
        
        GPVar *arg = new GPVar(m_compiler.newGP());
        m_arg_vars.push_back(arg);
        m_compiler.mov(*arg, imm((sysint_t)data));
        break;
      }
      case VT_VOID: break;
      default: break;
    }
  }
}

void FenrirInterpreter::NativeCallExecutor::setupArgs(ECall* ctx) {
  //setup args
  ctx->setPrototype(CALL_CONV_DEFAULT, m_native_fun_bldr);
  for (unsigned i = 0; i < m_arg_vars.size(); ++i) {
    ctx->setArgument(i, *m_arg_vars[i]);
  }
  
  switch (returnType()) {
    case VT_DOUBLE: {
      XMMVar ret(m_compiler.newXMM(VARIABLE_TYPE_XMM_1D));
      ctx->setReturn(ret);
      m_compiler.ret(ret);
      break;
    }
    case VT_VOID: break;
    default: {
      GPVar ret(m_compiler.newGP());
      ctx->setReturn(ret);
      m_compiler.ret(ret);
      break;
    }
  }
}
  
void FenrirInterpreter::NativeCallExecutor::performNativeCall() {
  //** Use power of AsmJit instead of generating asm by hands
  // inspired by http://fog-framework.blogspot.ru/2010/07/asmjit-function-calling-finally.html

  prepareReturnValue();
  m_compiler.getFunction()->setHint(FUNCTION_HINT_NAKED, true);
  prepareArgs();
 
  
  GPVar native_address(m_compiler.newGP());
  m_compiler.mov(native_address, imm((sysint_t)m_code_ref));
  setupArgs(m_compiler.call(native_address));
  
  m_compiler.endFunction();
  
  void *fn = m_compiler.make();
  if (!fn) {
    printf("Error making jit function (%u).\n", m_compiler.getError());
    exit(-1);
  }
  
  switch (returnType()) {
    case VT_DOUBLE: {
      double (*dfn)(void) = function_cast<double (*)(void)>(fn);
      double res = dfn();
      m_inter->m_op_stack.push(m_inter->d2i(res));
      break;
    }
    case VT_INT: {      
      uint64_t (*ifn)(void) = function_cast<uint64_t (*)(void)>(fn);
      uint64_t res = ifn();
      m_inter->m_op_stack.push(res);
      break;
    }
    case VT_VOID: {
      void (*vfn)(void) = function_cast<void (*)(void)>(fn);
      vfn();
      break;
    }
    case VT_STRING: {
      char *(*cfn)(void) = function_cast<char* (*)(void)>(fn);
      char * str = cfn();
      m_inter->m_op_stack.push(makeNativeStrDesc(str));
    }
    default: break;
  }
  
  MemoryManager::getGlobal()->free((void*)fn);
}


//Unused by translator commads
/*
DO(SLOAD0, "Load empty string on TOS.", 1)                      \
DO(IFICMPG, "Compare two topmost integers and jump if upper > lower, next two bytes - signed offset of jump destination.", 3) \
DO(IFICMPGE, "Compare two topmost integers and jump if upper >= lower, next two bytes - signed offset of jump destination.", 3) \
DO(IFICMPL, "Compare two topmost integers and jump if upper < lower, next two bytes - signed offset of jump destination.", 3) \
DO(IFICMPLE, "Compare two topmost integers and jump if upper <= lower, next two bytes - signed offset of jump destination.", 3) \
DO(POP, "Remove topmost value.", 1)                             \
DO(DUMP, "Dump value on TOS, without removing it.", 1)        \
DO(STOP, "Stop execution.", 1)                                  \
*/

