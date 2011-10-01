#include <iostream>
#include <stack>

#include "mathvm.h"
#include "BytecodeInterpreter.h"

// ================================================================================

namespace mathvm {

  // --------------------------------------------------------------------------------

  struct CallStackEntry {
    CallStackEntry(Bytecode* c, uint32_t ip) {
      oldCode = c;
      oldIP = ip;
    }

    void restore(Bytecode** c, uint32_t* ip) {
      *c = oldCode;
      *ip = oldIP;
    }

    Bytecode* oldCode;
    uint32_t oldIP;
  };

  static int opcode_len[] = {
#define OPCODE_LEN(b, d, l) l,
    FOR_BYTECODES(OPCODE_LEN)
#undef OPCODE_LEN
  };

  typedef int16_t offset_t;

  // --------------------------------------------------------------------------------

  Bytecode* BytecodeInterpreter::bytecode() {
    return &_code;
  }

  StringPool *BytecodeInterpreter::strings() {
    return &string_pool;
  }

  void BytecodeInterpreter::setVarPoolSize(unsigned int pool_sz) {
    _varPool.resize(pool_sz);
  }


  void BytecodeInterpreter::createFunction(Bytecode** code, uint16_t* id, FunArgs** args) {
    _functions.push_back(Function());
    *code = &_functions.back().code;
    *id = _functions.size() - 1;
    *args = &_functions.back().args;
  }
    

  Status* BytecodeInterpreter::execute(vector<Var*> vars) {
    Bytecode *code;
    uint32_t ip = 0;
    std::stack<RTVar> stack;
    std::stack<CallStackEntry> callStack;
    int stop = 0;

    code = &_code;

    while (ip < code->length() && !stop) {
      uint8_t opcode = code->get(ip);

      switch (opcode) {
        // Load instructions
      case BC_ILOAD:
        stack.push(RTVar(code->getInt64(ip + 1)));
        break;

      case BC_ILOAD0:
        stack.push(RTVar(0));
        break;

      case BC_ILOAD1:
        stack.push(RTVar(1));
        break;

      case BC_ILOADM1:
        stack.push(RTVar(-1));
        break;

      case BC_DLOAD:
        stack.push(RTVar(code->getDouble(ip + 1)));
        break;

      case BC_DLOAD0:
        stack.push(RTVar(0.0));
        break;

      case BC_DLOAD1:
        stack.push(RTVar(1.0));
        break;

      case BC_DLOADM1:
        stack.push(RTVar(-1.0));
        break;

      case BC_SLOAD:
        stack.push(RTVar((char*)string_pool[code->getInt16(ip + 1)].c_str()));
        break;

        // Arithmetics

      case BC_IADD: {
        int64_t op1, op2;

        op1 = stack.top().getInt(); stack.pop();
        op2 = stack.top().getInt(); stack.pop();
        stack.push(RTVar(op1 + op2));
      }
        break;

      case BC_ISUB: {
        int64_t op1, op2;

        op1 = stack.top().getInt(); stack.pop();
        op2 = stack.top().getInt(); stack.pop();
        stack.push(RTVar(op2 - op1));
      }
        break;

      case BC_IMUL: {
        int64_t op1, op2;

        op1 = stack.top().getInt(); stack.pop();
        op2 = stack.top().getInt(); stack.pop();
        stack.push(RTVar(op1*op2));
      }
        break;

      case BC_IDIV: {
        int64_t op1, op2;
        
        op1 = stack.top().getInt(); stack.pop();
        op2 = stack.top().getInt(); stack.pop();
        stack.push(RTVar(op2/op1));
      }
        break;

      case BC_INEG: {
        int64_t op;

        op = stack.top().getInt(); stack.pop();
        stack.push(RTVar(-op));
      }
        break;

        
      case BC_DADD: {
        double op1, op2;

        op1 = stack.top().getDouble(); stack.pop();
        op2 = stack.top().getDouble(); stack.pop();
        stack.push(RTVar(op1 + op2));
      }
        break;

      case BC_DSUB: {
        double op1, op2;

        op1 = stack.top().getDouble(); stack.pop();
        op2 = stack.top().getDouble(); stack.pop();
        stack.push(RTVar(op2 - op1));
      }
        break;

      case BC_DMUL: {
        double op1, op2;

        op1 = stack.top().getDouble(); stack.pop();
        op2 = stack.top().getDouble(); stack.pop();
        stack.push(RTVar(op1*op2));
      }
        break;

      case BC_DDIV: {
        double op1, op2;

        op1 = stack.top().getDouble(); stack.pop();
        op2 = stack.top().getDouble(); stack.pop();
        stack.push(RTVar(op2/op1));
      }
        break;

      case BC_DNEG: {
        double op;

        op = stack.top().getDouble(); stack.pop();
        stack.push(RTVar(-op));
      }
        break;

        // Conversion

      case BC_I2D: {
        int64_t op;

        op = stack.top().getInt(); stack.pop();
        stack.push(RTVar((double)op));
      }

      case BC_D2I: {
        double op;

        op = stack.top().getInt(); stack.pop();
        stack.push(RTVar((int64_t)op));
      }

        // Stack operations

      case BC_SWAP: {
        RTVar op1, op2;

        op1 = stack.top(); stack.pop();
        op2 = stack.top(); stack.pop();
        
        stack.push(op1);
        stack.push(op2);
      }
        break;

      case BC_POP:
        stack.pop();
        break;

        // Heap access

      case BC_LOADDVAR: 
      case BC_LOADIVAR: {
        int8_t id;

        id = code->get(ip + 1);
        stack.push(_varPool[id]);
      }
        break;

      case BC_STOREDVAR:
      case BC_STOREIVAR: {
        int8_t id;

        id = code->get(ip + 1);
        _varPool[id] = stack.top(); stack.pop();
      }
        break;

        // Comparision

      case BC_DCMP: {
        double op1, op2;

        op1 = stack.top().getDouble(); stack.pop();
        op2 = stack.top().getDouble(); stack.pop();
        
        if (op1 > op2) {
          stack.push(RTVar(1));
        } else if (op1 == op2) {
          stack.push(RTVar(0));
        } else {
          stack.push(RTVar(-1));
        }
      }
        break;

      case BC_ICMP: {
        int64_t op1, op2;

        op1 = stack.top().getInt(); stack.pop();
        op2 = stack.top().getInt(); stack.pop();
        
        if (op1 > op2) {
          stack.push(RTVar(1));
        } else if (op1 == op2) {
          stack.push(RTVar(0));
        } else {
          stack.push(RTVar(-1));
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

        op1 = stack.top().getInt(); stack.pop();
        op2 = stack.top().getInt(); stack.pop();

        if (op1 != op2) {        
          offset = code->getTyped<offset_t>(ip + 1);
          ip += offset;
        }
      }
        break;

      case BC_IFICMPE: {
        int64_t op1, op2;
        offset_t offset;

        op1 = stack.top().getInt(); stack.pop();
        op2 = stack.top().getInt(); stack.pop();

        if (op1 == op2) {        
          offset = code->getTyped<offset_t>(ip + 1);
          ip += offset;
        }
      }
        break;

      case BC_IFICMPG: {
        int64_t op1, op2;
        offset_t offset;
        
        op1 = stack.top().getInt(); stack.pop();
        op2 = stack.top().getInt(); stack.pop();

        if (op2 > op1) {        
          offset = code->getTyped<offset_t>(ip + 1);
          ip += offset;
        }
      }
        break;

      case BC_IFICMPGE: {
        int64_t op1, op2;
        offset_t offset;

        op1 = stack.top().getInt(); stack.pop();
        op2 = stack.top().getInt(); stack.pop();

        if (op2 >= op1) {        
          offset = code->getTyped<offset_t>(ip + 1);
          ip += offset;
        }
      }
        break;

      case BC_IFICMPL: {
        int64_t op1, op2;
        offset_t offset;

        op1 = stack.top().getInt(); stack.pop();
        op2 = stack.top().getInt(); stack.pop();

        if (op2 < op1) {        
          offset = code->getTyped<offset_t>(ip + 1);
          ip += offset;
        }
      }
        break;

      case BC_IFICMPLE: {
        int64_t op1, op2;
        offset_t offset;

        op1 = stack.top().getInt(); stack.pop();
        op2 = stack.top().getInt(); stack.pop();

        if (op2 <= op1) {        
          offset = code->getTyped<offset_t>(ip + 1);
          ip += offset;
        }
      }
        break;

      case BC_DUMP: {
        RTVar &v = stack.top();

        switch (v.type()) {
        case VT_INT:
          std::cout << v.getInt();
          break;

        case VT_DOUBLE:
          std::cout << v.getDouble();
          break;

        case VT_STRING:
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
        callStack.push(CallStackEntry(code, ip));
        code = &_functions[id].code;
        ip = 0;

        for (int i = _functions[id].args.size() - 1; i >= 0; --i) {
          _varPool[_functions[id].args[i].varId] = stack.top(); 
          stack.pop();
        }
      }
        continue;

      case BC_RETURN:
        callStack.top().restore(&code, &ip);
        callStack.pop();
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
