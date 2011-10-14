#include <iostream>
#include <stack>

#include <cstdlib>
#include <cassert>

#include "mathvm.h"
#include "BytecodeInterpreter.h"

// ================================================================================

namespace mathvm {

  /* TODO: Add function local scope --- the translator should
     figure out the scope variables */

  // --------------------------------------------------------------------------------

  static int opcode_len[] = {
#define OPCODE_LEN(b, d, l) l,
    FOR_BYTECODES(OPCODE_LEN)
#undef OPCODE_LEN
  };

  typedef int16_t offset_t;

  // --------------------------------------------------------------------------------

  Bytecode* BytecodeInterpreter::bytecode() {
    return NULL;
  }

  StringPool *BytecodeInterpreter::strings() {
    return &_stringPool;
  }

  void BytecodeInterpreter::setVarPoolSize(unsigned int pool_sz) {
    //_varPool.resize(pool_sz);
  }


  void BytecodeInterpreter::createFunction(Bytecode** code, uint16_t* id, FunArgs** args) {
    abort();
  }

  void BytecodeInterpreter::createFunction(BCIFunction** function, AstFunction* fNode) {
    _functions.push_back(new BCIFunction(fNode));
    *function = _functions.back();
  }
    
  void BytecodeInterpreter::createStackFrame() {
    const BCIFunction* f = _callStack.back().function();
    _stack.advance(f->localsNumber()*sizeof(RTVar));
  }

  void BytecodeInterpreter::leaveStackFrame() {
    const BCIFunction* f = _callStack.back().function();
    _stack.reduce((f->localsNumber() + f->parametersNumber())*sizeof(RTVar));
  }

  RTVar& BytecodeInterpreter::frameVar(uint16_t idx, const CallStackEntry& cse) {
    return _stack.get<RTVar>(cse.framePtr() + idx*sizeof(RTVar));
  }

  RTVar& BytecodeInterpreter::arg(uint16_t idx, const CallStackEntry& cse) {
    return _stack.get<RTVar>(cse.framePtr() + (idx - cse.function()->parametersNumber())*sizeof(RTVar));
  }

  RTVar& BytecodeInterpreter::var(uint16_t id) {
    for (CallStack::const_reverse_iterator csw = _callStack.rbegin();
         csw != _callStack.rend();
         ++csw) {

      const BCIFunction* curFun = csw->function();

      // Unwind the call stack to find the variable
      if (curFun->localsNumber() > 0 && 
          id >= curFun->firstLocal() && 
          id < curFun->firstLocal() + curFun->localsNumber()) {
        return frameVar(id - curFun->firstLocal(), *csw);
      } else if (curFun->parametersNumber() > 0 && 
                 id >= curFun->firstArg() && 
                 id < curFun->firstArg() + curFun->parametersNumber()) {
        return arg(id - curFun->firstArg(), *csw);
      }
    }

    abort();
  }


  Status* BytecodeInterpreter::execute(std::vector<mathvm::Var*, std::allocator<Var*> >& args) {
    const Bytecode *code;

    uint32_t ip = 0;
    //std::stack<CallStackEntry> callStack;
    int stop = 0;

    _stack.init();  // I don't know why the Stack contructor isn't called...
    _callStack.push_back(CallStackEntry(_functions[0], 0, 0));
    createStackFrame();
    code = _functions[0]->bytecode();

    while (ip < code->length() && !stop) {
      uint8_t opcode = code->get(ip);

      switch (opcode) {
        // Load instructions
      case BC_ILOAD:
        _stack.push(RTVar(code->getInt64(ip + 1)));
        break;

      case BC_ILOAD0:
        _stack.push(RTVar(0));
        break;

      case BC_ILOAD1:
        _stack.push(RTVar(1));
        break;

      case BC_ILOADM1:
        _stack.push(RTVar(-1));
        break;

      case BC_DLOAD:
        _stack.push(RTVar(code->getDouble(ip + 1)));
        break;

      case BC_DLOAD0:
        _stack.push(RTVar(0.0));
        break;

      case BC_DLOAD1:
        _stack.push(RTVar(1.0));
        break;

      case BC_DLOADM1:
        _stack.push(RTVar(-1.0));
        break;

      case BC_SLOAD:
        _stack.push(RTVar((char*)_stringPool[code->getInt16(ip + 1)].c_str()));
        break;

        // Arithmetics

      case BC_IADD: {
        int64_t op1, op2;

        op1 = _stack.top().getInt(); _stack.pop();
        op2 = _stack.top().getInt(); _stack.pop();
        _stack.push(RTVar(op1 + op2));
      }
        break;

      case BC_ISUB: {
        int64_t op1, op2;

        op1 = _stack.top().getInt(); _stack.pop();
        op2 = _stack.top().getInt(); _stack.pop();
        _stack.push(RTVar(op2 - op1));
      }
        break;

      case BC_IMUL: {
        int64_t op1, op2;

        op1 = _stack.top().getInt(); _stack.pop();
        op2 = _stack.top().getInt(); _stack.pop();
        _stack.push(RTVar(op1*op2));
      }
        break;

      case BC_IDIV: {
        int64_t op1, op2;
        
        op1 = _stack.top().getInt(); _stack.pop();
        op2 = _stack.top().getInt(); _stack.pop();
        _stack.push(RTVar(op2/op1));
      }
        break;

      case BC_INEG: {
        int64_t op;

        op = _stack.top().getInt(); _stack.pop();
        _stack.push(RTVar(-op));
      }
        break;

        
      case BC_DADD: {
        double op1, op2;

        op1 = _stack.top().getDouble(); _stack.pop();
        op2 = _stack.top().getDouble(); _stack.pop();
        _stack.push(RTVar(op1 + op2));
      }
        break;

      case BC_DSUB: {
        double op1, op2;

        op1 = _stack.top().getDouble(); _stack.pop();
        op2 = _stack.top().getDouble(); _stack.pop();
        _stack.push(RTVar(op2 - op1));
      }
        break;

      case BC_DMUL: {
        double op1, op2;

        op1 = _stack.top().getDouble(); _stack.pop();
        op2 = _stack.top().getDouble(); _stack.pop();
        _stack.push(RTVar(op1*op2));
      }
        break;

      case BC_DDIV: {
        double op1, op2;

        op1 = _stack.top().getDouble(); _stack.pop();
        op2 = _stack.top().getDouble(); _stack.pop();
        _stack.push(RTVar(op2/op1));
      }
        break;

      case BC_DNEG: {
        double op;

        op = _stack.top().getDouble(); _stack.pop();
        _stack.push(RTVar(-op));
      }
        break;

        // Conversion

      case BC_I2D: {
        int64_t op;

        op = _stack.top().getInt(); _stack.pop();
        _stack.push(RTVar((double)op));
      }
        break;

      case BC_D2I: {
        double op;

        op = _stack.top().getInt(); _stack.pop();
        _stack.push(RTVar((int64_t)op));
      }
        break;

        // Stack operations

      case BC_SWAP: {
        RTVar op1, op2;

        op1 = _stack.top(); _stack.pop();
        op2 = _stack.top(); _stack.pop();
        
        _stack.push(op1);
        _stack.push(op2);
      }
        break;

      case BC_POP:
        _stack.pop();
        break;

        // Heap access

      case BC_LOADDVAR: 
      case BC_LOADIVAR: {
        uint16_t id;

        id = code->getUInt16(ip + 1);
        _stack.push(var(id));
      }
        break;

      case BC_STOREDVAR:
      case BC_STOREIVAR: {
        uint16_t id;

        id = code->getUInt16(ip + 1);
        var(id) = _stack.pop<RTVar>();
      }
        break;

        // Comparision

      case BC_DCMP: {
        double op1, op2;

        op1 = _stack.top().getDouble(); _stack.pop();
        op2 = _stack.top().getDouble(); _stack.pop();
        
        if (op1 > op2) {
          _stack.push(RTVar(1));
        } else if (op1 == op2) {
          _stack.push(RTVar(0));
        } else {
          _stack.push(RTVar(-1));
        }
      }
        break;

      case BC_ICMP: {
        int64_t op1, op2;

        op1 = _stack.top().getInt(); _stack.pop();
        op2 = _stack.top().getInt(); _stack.pop();
        
        if (op1 > op2) {
          _stack.push(RTVar(1));
        } else if (op1 == op2) {
          _stack.push(RTVar(0));
        } else {
          _stack.push(RTVar(-1));
        }
      }
        break;

        // Jumps

      case BC_JA: {
        offset_t offset = code->getTyped<offset_t>(ip + 1);
        ip += offset;        
      }
        break;

      case BC_IFICMPNE: {
        int64_t op1, op2;
        offset_t offset;

        op1 = _stack.top().getInt(); _stack.pop();
        op2 = _stack.top().getInt(); _stack.pop();

        if (op1 != op2) {        
          offset = code->getTyped<offset_t>(ip + 1);
          ip += offset;
        }
      }
        break;

      case BC_IFICMPE: {
        int64_t op1, op2;
        offset_t offset;

        op1 = _stack.top().getInt(); _stack.pop();
        op2 = _stack.top().getInt(); _stack.pop();

        if (op1 == op2) {        
          offset = code->getTyped<offset_t>(ip + 1);
          ip += offset;
        }
      }
        break;

      case BC_IFICMPG: {
        int64_t op1, op2;
        offset_t offset;
        
        op1 = _stack.top().getInt(); _stack.pop();
        op2 = _stack.top().getInt(); _stack.pop();

        if (op2 > op1) {        
          offset = code->getTyped<offset_t>(ip + 1);
          ip += offset;
        }
      }
        break;

      case BC_IFICMPGE: {
        int64_t op1, op2;
        offset_t offset;

        op1 = _stack.top().getInt(); _stack.pop();
        op2 = _stack.top().getInt(); _stack.pop();

        if (op2 >= op1) {        
          offset = code->getTyped<offset_t>(ip + 1);
          ip += offset;
        }
      }
        break;

      case BC_IFICMPL: {
        int64_t op1, op2;
        offset_t offset;

        op1 = _stack.top().getInt(); _stack.pop();
        op2 = _stack.top().getInt(); _stack.pop();

        if (op2 < op1) {        
          offset = code->getTyped<offset_t>(ip + 1);
          ip += offset;
        }
      }
        break;

      case BC_IFICMPLE: {
        int64_t op1, op2;
        offset_t offset;

        op1 = _stack.top().getInt(); _stack.pop();
        op2 = _stack.top().getInt(); _stack.pop();

        if (op2 <= op1) {        
          offset = code->getTyped<offset_t>(ip + 1);
          ip += offset;
        }
      }
        break;

      case BC_DUMP: {
        RTVar &v = _stack.top();

        switch (v.type()) {
        case RVT_INT:
          std::cout << v.getInt();
          break;

        case RVT_DOUBLE:
          std::cout << v.getDouble();
          break;

        case RVT_STRING:
          std::cout << v.getString();
          break;
        
        default:
          break;
        }
      }
        break;

      case BC_CALL: {
        uint16_t id = code->getUInt16(ip + 1);
        BCIFunction* f = _functions[id];
        size_t framePos = _stack.pos();

        ip += 3;
        _callStack.back().oldIP = ip;
        _callStack.push_back(CallStackEntry(f, 0, framePos));
        createStackFrame();

        code = f->bytecode();
        ip = 0;        
      }
        continue;

      case BC_RETURN: {
        RTVar ret;
        const BCIFunction* f = _callStack.back().function();
        
        if (f->returnType() != VT_VOID) {
          ret = _stack.pop();
          leaveStackFrame();
          _stack.push(ret);
        } else {
          leaveStackFrame();
        }

        _callStack.pop_back();
        ip = _callStack.back().oldIP;
        

        // Sorry, const...
        code = ((BCIFunction*)_callStack.back().function())->bytecode();
      }
        continue;

      case BC_STOP:
        stop = 1;
        break;

      default:
        assert(0);
        break;
      }

      ip += opcode_len[opcode];
    }

    return 0;
  }
};
