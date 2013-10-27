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
  
  m_func_intimates[fn->scopeId()].push(std::vector<int64_t>(fn->localsNumber()));
  
  locals = &m_func_intimates[fn->scopeId()].top();
  
  //init params
  for(int32_t param = fn->parametersNumber(); param > 0; --param) {
    setVarToCtx(fn->scopeId(), param, m_op_stack.top());
    m_op_stack.pop();
  }
  
}

void FenrirInterpreter::functionCallEpilogue() {
  m_func_intimates[fn->scopeId()].pop();
  
  std::pair<uint32_t, BytecodeFunction *> stacked = m_call_stack.top();
  m_call_stack.pop();
  
  fn = stacked.second;
  ic = stacked.first;
  bc = fn->bytecode();
  locals = &m_func_intimates[fn->scopeId()].top();
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
        double val = i2d(m_op_stack.top());
        m_op_stack.pop();
        m_op_stack.push(d2i(-val));
        break;
      }
      case BC_INEG: {
        int64_t val = m_op_stack.top();
        m_op_stack.pop();
        m_op_stack.push(-val);
        break;
      }
      case BC_DPRINT:
        std::cout << i2d(m_op_stack.top());
        m_op_stack.pop();
        break;
      case BC_IPRINT:
        std::cout << (int64_t) m_op_stack.top();
        m_op_stack.pop();
        break;
      case BC_SWAP: {
        int64_t val1 = m_op_stack.top();
        m_op_stack.pop();
        int64_t val2 = m_op_stack.top();
        m_op_stack.pop();
        m_op_stack.push(val1);
        m_op_stack.push(val2);
        break;
      }
      case BC_LOADDVAR:
        m_op_stack.push(locals->operator[](bc->getInt16(ic)));
        ic += 2;
        break;
      case BC_LOADIVAR:
        m_op_stack.push(locals->operator[](bc->getInt16(ic)));
        ic += 2;
        break;
      case BC_STOREDVAR:
        locals->operator[](bc->getInt16(ic)) = m_op_stack.top();
        ic += 2;
        m_op_stack.pop();
        break;
      case BC_STOREIVAR:
        locals->operator[](bc->getInt16(ic)) = m_op_stack.top();
        ic += 2;
        m_op_stack.pop();
        break;
      case BC_JA:
        ic += bc->getInt16(ic);
        break;
      case BC_IFICMPE: {
        int64_t val1 = m_op_stack.top();
        m_op_stack.pop();
        int64_t val2 = m_op_stack.top();
        m_op_stack.pop();
        if (val1 == val2) { ic += bc->getInt16(ic); }
        else { ic += 2; }
        break;
      }
      case BC_IFICMPNE: {
        int64_t val1 = m_op_stack.top();
        m_op_stack.pop();
        int64_t val2 = m_op_stack.top();
        m_op_stack.pop();
        if (val1 != val2) { ic += bc->getInt16(ic); }
        else { ic += 2; }
        break;
      }
      
      case BC_DCMP: {
        double val1 = i2d(m_op_stack.top());
        m_op_stack.pop();
        double val2 = i2d(m_op_stack.top());
        m_op_stack.pop();
        m_op_stack.push(val1 == val2 ? 0 : val1 > val2 ? 1 : -1);
        break;
      }
      case BC_ICMP: {
        int64_t val1 = m_op_stack.top();
        m_op_stack.pop();
        int64_t val2 = m_op_stack.top();
        m_op_stack.pop();
        m_op_stack.push(val1 == val2 ? 0 : val1 > val2 ? 1 : -1);
        break;
      }
      case BC_SLOAD: {
        uint16_t s_id = bc->getInt16(ic);
        ic += 2;
        m_op_stack.push(s_id);
        break;
      }
      case BC_SPRINT: {
        uint16_t s_id = m_op_stack.top();
        m_op_stack.pop();
        std::cout << constantById(s_id);
        break;
      }
      case BC_LOADSVAR: {
        uint16_t loc = bc->getInt16(ic);
        ic += 2;
        m_op_stack.push(locals->operator[](loc));
        break;
      }
      case BC_STORESVAR: {
        uint16_t s_id = m_op_stack.top();
        m_op_stack.pop();
        uint16_t loc = bc->getInt16(ic);
        ic += 2;
        locals->operator[](loc) = s_id;
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
        int64_t val = m_op_stack.top();
        setVarToCtx(ctx_id, local_id, val);
        m_op_stack.pop();
        break;
      }
      case BC_D2I: {
        double v = d2i(m_op_stack.top());
        m_op_stack.pop();
        m_op_stack.push(v);
        
        break;
      }
      case BC_I2D: {
        double v = m_op_stack.top();
        m_op_stack.pop();
        m_op_stack.push(d2i(v));
        break;
      }

      default:
        break;
    }

  }

}


/*
DO(LOADCTXSVAR, "Load string from variable, whose 2-byte context and 2-byte id is inlined to insn stream, push on TOS.", 5) \
DO(STORECTXSVAR, "Pop TOS and store to string variable, whose 2-byte context and 2-byte id is inlined to insn stream.", 5) \

DO(SLOAD0, "Load empty string on TOS.", 1)                      \
DO(I2D,  "Convert int on TOS to double.", 1)                    \
DO(D2I,  "Convert double on TOS to int.", 1)                    \
DO(S2I,  "Convert string on TOS to int.", 1)                    \
*/

/*
 DO(CALLNATIVE, "Call native function, next two bytes - id of the native function.", 3)  \
DO(IFICMPG, "Compare two topmost integers and jump if upper > lower, next two bytes - signed offset of jump destination.", 3) \
DO(IFICMPGE, "Compare two topmost integers and jump if upper >= lower, next two bytes - signed offset of jump destination.", 3) \
DO(IFICMPL, "Compare two topmost integers and jump if upper < lower, next two bytes - signed offset of jump destination.", 3) \
DO(IFICMPLE, "Compare two topmost integers and jump if upper <= lower, next two bytes - signed offset of jump destination.", 3) \
DO(POP, "Remove topmost value.", 1)                             \
DO(LOADDVAR0, "Load double from variable 0, push on TOS.", 1)   \
DO(LOADDVAR1, "Load double from variable 1, push on TOS.", 1)   \
DO(LOADDVAR2, "Load double from variable 2, push on TOS.", 1)   \
DO(LOADDVAR3, "Load double from variable 3, push on TOS.", 1)   \
DO(LOADIVAR0, "Load int from variable 0, push on TOS.", 1)      \
DO(LOADIVAR1, "Load int from variable 1, push on TOS.", 1)      \
DO(LOADIVAR2, "Load int from variable 2, push on TOS.", 1)      \
DO(LOADIVAR3, "Load int from variable 3, push on TOS.", 1)      \
DO(LOADSVAR0, "Load string from variable 0, push on TOS.", 1)   \
DO(LOADSVAR1, "Load string from variable 1, push on TOS.", 1)   \
DO(LOADSVAR2, "Load string from variable 2, push on TOS.", 1)   \
DO(LOADSVAR3, "Load string from variable 3, push on TOS.", 1)   \
DO(STOREDVAR0, "Pop TOS and store to double variable 0.", 1)    \
DO(STOREDVAR1, "Pop TOS and store to double variable 1.", 1)    \
DO(STOREDVAR2, "Pop TOS and store to double variable 0.", 1)    \
DO(STOREDVAR3, "Pop TOS and store to double variable 3.", 1)    \
DO(STOREIVAR0, "Pop TOS and store to int variable 0.", 1)       \
DO(STOREIVAR1, "Pop TOS and store to int variable 1.", 1)       \
DO(STOREIVAR2, "Pop TOS and store to int variable 0.", 1)       \
DO(STOREIVAR3, "Pop TOS and store to int variable 3.", 1)       \
DO(STORESVAR0, "Pop TOS and store to string variable 0.", 1)    \
DO(STORESVAR1, "Pop TOS and store to string variable 1.", 1)    \
DO(STORESVAR2, "Pop TOS and store to string variable 0.", 1)    \
DO(STORESVAR3, "Pop TOS and store to string variable 3.", 1)    \
DO(LOADCTXDVAR, "Load double from variable, whose 2-byte context and 2-byte id inlined to insn stream, push on TOS.", 5) \
DO(LOADCTXIVAR, "Load int from variable, whose 2-byte context and 2-byte id is inlined to insn stream, push on TOS.", 5) \
DO(LOADCTXSVAR, "Load string from variable, whose 2-byte context and 2-byte id is inlined to insn stream, push on TOS.", 5) \
DO(STORECTXDVAR, "Pop TOS and store to double variable, whose 2-byte context and 2-byte id is inlined to insn stream.", 5) \
DO(STORECTXIVAR, "Pop TOS and store to int variable, whose 2-byte context and 2-byte id is inlined to insn stream.", 5) \
DO(STORECTXSVAR, "Pop TOS and store to string variable, whose 2-byte context and 2-byte id is inlined to insn stream.", 5) \
DO(DUMP, "Dump value on TOS, without removing it.", 1)        \
DO(STOP, "Stop execution.", 1)                                  \
*/

