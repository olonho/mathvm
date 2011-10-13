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

  struct CallStackEntry {
    CallStackEntry(BCIFunction* f, uint32_t ip, size_t frame) {
      oldFunction = f;
      oldIP = ip;
      oldFrame = frame;
    }

    void restore(BCIFunction** f, uint32_t* ip, size_t* frame) {
      *f = oldFunction;
      *ip = oldIP;
      *frame = oldFrame;
    }

    BCIFunction* oldFunction;
    uint32_t oldIP;
    size_t oldFrame;
    //std::vector<RTVar> locals;
  };

  static int opcode_len[] = {
#define OPCODE_LEN(b, d, l) l,
    FOR_BYTECODES(OPCODE_LEN)
#undef OPCODE_LEN
  };

  typedef int16_t offset_t;

  // --------------------------------------------------------------------------------

  Bytecode* BytecodeInterpreter::bytecode() {
    return NULL;
    //return &_code;
  }

  StringPool *BytecodeInterpreter::strings() {
    return &_stringPool;
  }

  void BytecodeInterpreter::setVarPoolSize(unsigned int pool_sz) {
    _varPool.resize(pool_sz);
  }


  void BytecodeInterpreter::createFunction(Bytecode** code, uint16_t* id, FunArgs** args) {
    abort();

    /*
    _functions.push_back(Function());
    *code = &_functions.back().code;
    *id = _functions.size() - 1;
    *args = &_functions.back().args;
    */
  }

  void BytecodeInterpreter::createFunction(BCIFunction** function, AstFunction* fNode) {
    _functions.push_back(new BCIFunction(fNode));
    *function = _functions.back();
  }
    

  //typedef std::vector<RTVar> Stack;

  void BytecodeInterpreter::createStackFrame(BCIFunction* f) {
    /*
    for (size_t i = 0; i < f->localsNumber(); ++i) {
      _stack.push(_varPool[f->firstLocal() + i]);
      }*/
    _stack.advance(f->localsNumber()*sizeof(RTVar));
  }

  void BytecodeInterpreter::leaveStackFrame(BCIFunction* f) {
    _stack.reduce((f->localsNumber() + f->parametersNumber())*sizeof(RTVar));
  }

  RTVar& BytecodeInterpreter::frameVar(uint16_t idx) {
    return _stack.get<RTVar>(_framePos + idx*sizeof(RTVar));
  }

  RTVar& BytecodeInterpreter::arg(uint16_t idx) {
    return _stack.get<RTVar>(_framePos + (idx - _curFun->parametersNumber())*sizeof(RTVar));
  }


  Status* BytecodeInterpreter::execute(std::vector<mathvm::Var*, std::allocator<Var*> >& args) {
    Bytecode *code;

    uint32_t ip = 0;
    std::stack<CallStackEntry> callStack;
    int stop = 0;

    //_stack.init();
    _curFun = _functions[0];
    _framePos = 0;

    BCIFunction* top = _functions[0];
    createStackFrame(top);
    code = top->bytecode();

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
        if (_curFun->localsNumber() > 0 && 
            id >= _curFun->firstLocal() && 
            id < _curFun->firstLocal() + _curFun->localsNumber()) {
          _stack.push(frameVar(id - _curFun->firstLocal()));
        } else {
          if (_curFun->parametersNumber() > 0 && 
              id >= _curFun->firstArg() && 
              id < _curFun->firstArg() + _curFun->parametersNumber()) {
            _stack.push(arg(id - _curFun->firstArg()));
          } else {
            std::cout << "References to the outer lexical scope are not supported yet" << std::endl;
            abort();
          }
        }
      }
        break;

      case BC_STOREDVAR:
      case BC_STOREIVAR: {
        uint16_t id;

        id = code->getUInt16(ip + 1);
        if (_curFun->localsNumber() > 0 && 
            id >= _curFun->firstLocal() &&
            id < _curFun->firstLocal() + _curFun->localsNumber()) {
          // A local variable is refereced

          frameVar(id - _curFun->firstLocal()) = _stack.pop<RTVar>();
        } else {
          // An argument or a lexical scope variable is referenced

          std::cout << "References to the outer lexical scope are not supported yet" << std::endl;
          abort();

          /*
          if (curFun->parametersNumber() > 0 && id - curFun->parametersNumber() > 0) {
            
          }
          */
        }
        //_varPool[id] = _stack.top(); _stack.pop();
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

        ip += 3;
        callStack.push(CallStackEntry(_curFun, ip, _framePos));
        _curFun = _functions[id];
        code = _functions[id]->bytecode();
        _framePos = _stack.pos();
        createStackFrame(_curFun);
        ip = 0;        
        
        /*
        for (int i = _functions[id].args.size() - 1; i >= 0; --i) {
          _varPool[_functions[id].args[i].varId] = _stack.top(); 
          _stack.pop();
        }
        */
      }
        continue;

      case BC_RETURN: {
        RTVar ret;
        
        if (_curFun->returnType() != VT_VOID) {
          ret = _stack.pop();
        }

        leaveStackFrame(_curFun);
        if (_curFun->returnType() != VT_VOID) {
          _stack.push(ret);
        }

        callStack.top().restore(&_curFun, &ip, &_framePos);
        code = _curFun->bytecode();
        callStack.pop();        
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
