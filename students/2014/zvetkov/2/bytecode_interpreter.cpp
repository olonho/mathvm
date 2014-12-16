#include "bytecode_interpreter.hpp"
#include "errors.hpp"

#include <iostream>

#define BIN_OP(type, op) {    \
  type upper = pop<type>();   \
  type lower = pop<type>();   \
  push<type>(upper op lower); \
}

#define CMP(type) {                         \
  type upper = pop<type>();                 \
  type lower = pop<type>();                 \
  if (upper == lower) push<int64_t>(0);     \
  else if (upper < lower) push<int64_t>(-1);\
  else push<int64_t>(1);                    \
}

#define CMP_OP(op, ip, off_t) {     \
  int64_t upper = pop<int64_t>();   \
  int64_t lower = pop<int64_t>();   \
  if (upper op lower) {             \
    ip += getTyped<off_t>();        \
  } else {                          \
    ip += sizeof(off_t);            \
  }                                 \
}                                

namespace mathvm {

BytecodeInterpreter::BytecodeInterpreter(Code* code)
  : instructionPointer_(0), 
    stackPointer_(0), 
    stackFramePointer_(constants::MAX_STACK_SIZE)
{
  stack_ = new char[constants::MAX_STACK_SIZE];
  code_ = dynamic_cast<InterpreterCodeImpl*>(code);
  assert(code_ != NULL);
  function_ = code_->functionById(0);
  allocFrame(0, function_->localsNumber(), -1);
}

BytecodeInterpreter::~BytecodeInterpreter() {
  delete [] stack_;
}

void BytecodeInterpreter::execute() {
  while (true) {
    Instruction bci = bc()->getInsn(instructionPointer_++);

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

      case BC_DCMP: CMP(double); break;
      case BC_ICMP: CMP(int64_t); break;

      case BC_I2D: push((double)  pop<int64_t>()); break;
      case BC_D2I: push((int64_t) pop<double>()); break;

      case BC_DNEG: push(-pop<double>()); break;
      case BC_INEG: push(-pop<int64_t>()); break;

      case BC_JA: instructionPointer_ += getTyped<int16_t>(); break;
      case BC_IFICMPNE: CMP_OP(!=, instructionPointer_, int16_t); break;
      case BC_IFICMPE:  CMP_OP(==, instructionPointer_, int16_t); break;
      case BC_IFICMPG:  CMP_OP(>,  instructionPointer_, int16_t); break;
      case BC_IFICMPGE: CMP_OP(>=, instructionPointer_, int16_t); break;
      case BC_IFICMPL:  CMP_OP(<,  instructionPointer_, int16_t); break;
      case BC_IFICMPLE: CMP_OP(<=, instructionPointer_, int16_t); break;

      case BC_LOADIVAR: loadVar<int64_t, LOCAL>(); break;
      case BC_LOADDVAR: loadVar<double, LOCAL>(); break;
      case BC_LOADCTXIVAR: loadVar<int64_t, OUTER>(); break;
      case BC_LOADCTXDVAR: loadVar<double, OUTER>(); break;

      case BC_STOREIVAR: storeVar<int64_t, LOCAL>(); break;
      case BC_STOREDVAR: storeVar<double, LOCAL>(); break;
      case BC_STORECTXIVAR: storeVar<int64_t, OUTER>(); break;
      case BC_STORECTXDVAR: storeVar<double, OUTER>(); break;

      case BC_CALL: callFunction(getTypedAndMoveNext<uint16_t>()); break;
      case BC_RETURN: returnFunction(); break;
      case BC_SWAP: swap(); break;
      case BC_POP: remove(); break;
      case BC_STOP: return;
      
      default: throw InterpreterException("Not implemented instruction");
    }
  } // while
} // execute


StackFrame* BytecodeInterpreter::stackFrame() { 
  return reinterpret_cast<StackFrame*>(stack_ + stackFramePointer_); 
}

/*
 * functionContext is difference between
 * current function deepness and called function deepness
 * 
 * For example: 
 *   function void f() {
 *     function void g() {
 *       f();
 *     }
 *
 *     g();
 *   }
 * For call g() from f context is -1;
 * for call f() from g context is 1.
 */
void BytecodeInterpreter::allocFrame(uint16_t functionId, uint32_t localsNumber, int64_t context) {
  mem_t parentFrame;
  assert(context >= -1);

  if (context == -1) {
    parentFrame = stackFramePointer_;
  } else {
    mem_t sf = stackFramePointer_;

    for (int64_t i = 0; i < context; ++i) {
      stackFramePointer_ = stackFrame()->parentFrame();
    }

    parentFrame = stackFrame()->parentFrame();
    stackFramePointer_ = sf;
  }

  mem_t returnFrame = stackFramePointer_;
  stackFramePointer_ -= (sizeof(StackFrame) + constants::VAL_SIZE * localsNumber);
  *stackFrame() = StackFrame(function_->id(), 
                             instructionPointer_, 
                             parentFrame,
                             returnFrame);
}

void BytecodeInterpreter::callFunction(uint16_t id) {
  BytecodeFunction* called = code_->functionById(id);
  allocFrame(called->id(), called->localsNumber(), pop<int64_t>());
  function_ = called;
  instructionPointer_ = 0;
} 

void BytecodeInterpreter::returnFunction() {
  uint64_t returnValue = pop<uint64_t>();
  StackFrame* frame = stackFrame();
  instructionPointer_ = frame->instruction();
  stackFramePointer_  = frame->returnFrame();
  function_ = code_->functionById(frame->function());
  push(returnValue);
}

} // namespace mathvm