#include "bytecode_interpreter.hpp"
#include "errors.hpp"

#include <iostream>

#define BIN_OP(type, op) {    \
  type upper = pop<type>();   \
  type lower = pop<type>();   \
  push<type>(upper op lower); \
}

#define CMP_OP(op, ip, off_t) {     \
  int64_t upper = pop<int64_t>();   \
  int64_t lower = pop<int64_t>();   \
  if (upper op lower) {             \
    ip += readFromBc<off_t>();      \
  } else {                          \
    ip += sizeof(off_t);            \
  }                                 \
}                                

namespace mathvm {

void BytecodeInterpreter::execute() {
  while (ins_ < bc()->length()) {
    Instruction bci = bc()->getInsn(ins_++);

    switch (bci) {
      case BC_INVALID: 
        throw InterpreterException("Not implemented bytecode: %s", bytecodeName(bci, 0));
      
      case BC_ILOAD0: push<int64_t>(0); break;      
      case BC_ILOAD1: push<int64_t>(1); break;      
      case BC_ILOADM1: push<int64_t>(-1); break;      
      case BC_DLOAD0: push<double>(0); break;      
      case BC_DLOAD1: push<double>(1); break;      
      case BC_DLOADM1: push<double>(-1); break;      

      case BC_ILOAD: load<int64_t>(); break;
      case BC_DLOAD: load<double>(); break;
      case BC_SLOAD: load<uint16_t>(); break;
      
      case BC_IPRINT: std::cout << pop<int64_t>(); break;
      case BC_DPRINT: std::cout << pop<double>(); break;
      case BC_SPRINT: std::cout << code_->constantById(pop<uint16_t>()); break;

      case BC_DADD: BIN_OP(double, +); break;
      case BC_DSUB: BIN_OP(double, -); break;
      case BC_DMUL: BIN_OP(double, *); break;
      case BC_DDIV: BIN_OP(double, /); break;

      case BC_IADD: BIN_OP(int64_t, +); break;
      case BC_ISUB: BIN_OP(int64_t, -); break;
      case BC_IMUL: BIN_OP(int64_t, *); break;
      case BC_IDIV: BIN_OP(int64_t, /); break;
      case BC_IMOD: BIN_OP(int64_t, %); break;
      case BC_IAOR: BIN_OP(int64_t, |); break;
      case BC_IAAND: BIN_OP(int64_t, &); break;
      case BC_IAXOR: BIN_OP(int64_t, ^); break;

      case BC_DCMP: 
        swap();
        BIN_OP(double, -); 
        push((int64_t) pop<double>()); 
        break;
      case BC_ICMP: swap(); BIN_OP(int64_t, -); break;

      case BC_I2D: push((double)  pop<int64_t>()); break;
      case BC_D2I: push((int64_t) pop<double>()); break;

      case BC_DNEG: push(-pop<double>()); break;
      case BC_INEG: push(-pop<int64_t>()); break;

      case BC_JA: ins_ += readFromBc<int16_t>(); break;
      case BC_IFICMPNE: CMP_OP(!=, ins_, int16_t); break;
      case BC_IFICMPE:  CMP_OP(==, ins_, int16_t); break;
      case BC_IFICMPG:  CMP_OP(>,  ins_, int16_t); break;
      case BC_IFICMPGE: CMP_OP(>=, ins_, int16_t); break;
      case BC_IFICMPL:  CMP_OP(<,  ins_, int16_t); break;
      case BC_IFICMPLE: CMP_OP(<=, ins_, int16_t); break;

      case BC_LOADIVAR: 
        loadVar<int64_t>(readFromBcAndShift<uint16_t>(), 0); 
        break;
      case BC_LOADDVAR: 
        loadVar<double>(readFromBcAndShift<uint16_t>(), 0); 
        break;
      case BC_LOADCTXIVAR: 
        loadVar<int64_t>(readFromBcAndShift<uint16_t>(), readFromBcAndShift<uint16_t>()); 
        break;
      case BC_LOADCTXDVAR: 
        loadVar<double>(readFromBcAndShift<uint16_t>(), readFromBcAndShift<uint16_t>()); 
        break;

      case BC_STOREIVAR: 
        storeVar<int64_t>(readFromBcAndShift<uint16_t>(), 0, pop<int64_t>()); 
        break;
      case BC_STOREDVAR: 
        storeVar<double>(readFromBcAndShift<uint16_t>(), 0, pop<double>()); 
        break;
      case BC_STORECTXIVAR: 
        storeVar<int64_t>(readFromBcAndShift<uint16_t>(), readFromBcAndShift<uint16_t>(), pop<int64_t>()); 
        break;
      case BC_STORECTXDVAR: 
        storeVar<double>(readFromBcAndShift<uint16_t>(), readFromBcAndShift<uint16_t>(), pop<double>()); 
        break;

      case BC_CALL: callFunction(readFromBcAndShift<uint16_t>()); break;
      case BC_RETURN: returnFunction(); break;
      case BC_SWAP: swap(); break;
      case BC_POP: remove(); break;
      case BC_STOP: return;
      
      default: throw InterpreterException("Not implemented instruction");
    }
  } // while
}

} // namespace mathvm