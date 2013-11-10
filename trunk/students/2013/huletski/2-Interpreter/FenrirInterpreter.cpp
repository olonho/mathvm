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
  
  locals = &getVectorFromCache(m_func_intimates[fn->scopeId()].top());
  
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
  locals = &getVectorFromCache(m_func_intimates[fn->scopeId()].top());
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
      case BC_DLOAD0: m_op_stack.push(0); break;
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
        std::cout << constantById((uint16_t)m_op_stack.pop());
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
      default:
        break;
    }

  }

}

//To be imlemented
/*
DO(CALLNATIVE, "Call native function, next two bytes - id of the native function.", 3)  \
*/

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

