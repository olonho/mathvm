#include <stack>

#include "mathvm.h"
#include "BytecodeInterpreter.h"

// ================================================================================

namespace mathvm {
  RTVar pack(int i) {
    RTVar v;

    v.i = i;
    return v;
  }

  RTVar pack(int64_t i) {
    RTVar v;

    v.i = i;
    return v;
  }

  RTVar pack(double d) {
    RTVar v;

    v.d = d;
    return v;
  }

  RTVar pack(char *s) {
    RTVar v;

    v.s = s;
    return v;
  }

  int64_t unpackI(RTVar v) {
    return v.i;
  }

  double unpackD(RTVar v) {
    return v.d;
  }
  
  char* unpackS(RTVar v) {
    return v.s;
  }

  // --------------------------------------------------------------------------------

  Bytecode* BytecodeInterpreter::bytecode() {
    return &code;
  }

  StringPool *BytecodeInterpreter::strings() {
    return &string_pool;
  }

  void BytecodeInterpreter::setVarPoolSize(unsigned int pool_sz) {
    var_pool.reserve(pool_sz);
  }

  static int opcode_len[] = {
#define OPCODE_LEN(b, d, l) l,
    FOR_BYTECODES(OPCODE_LEN)
#undef OPCODE_LEN
  };
    

  Status* BytecodeInterpreter::execute(vector<Var*> vars) {
    uint32_t ip = 0;
    std::stack<RTVar> stack;

    while (ip <= code.length()) {
      uint8_t opcode = code.get(ip);

      switch (opcode) {
        // Load instructions
      case BC_ILOAD:
        stack.push(pack(code.getInt64(ip + 1)));
        break;

      case BC_ILOAD0:
        stack.push(pack(0));
        break;

      case BC_ILOAD1:
        stack.push(pack(1));
        break;

      case BC_ILOADM1:
        stack.push(pack(-1));
        break;


      case BC_DLOAD:
        stack.push(pack(code.getDouble(ip + 1)));
        break;

      case BC_DLOAD0:
        stack.push(pack(0.0));
        break;

      case BC_DLOAD1:
        stack.push(pack(1.0));
        break;

      case BC_DLOADM1:
        stack.push(pack(-1.0));
        break;

      case BC_SLOAD:
        stack.push(pack((char*)code.getInt64(ip + 1)));
        break;

        // Arithmetics

      case BC_IADD: {
        int64_t op1, op2;

        op1 = unpackI(stack.top()); stack.pop();
        op2 = unpackI(stack.top()); stack.pop();
        stack.push(pack(op1 + op2));
      }
        break;

      case BC_ISUB: {
        int64_t op1, op2;

        op1 = unpackI(stack.top()); stack.pop();
        op2 = unpackI(stack.top()); stack.pop();
        stack.push(pack(op1 - op2));
      }
        break;

      case BC_IMUL: {
        int64_t op1, op2;

        op1 = unpackI(stack.top()); stack.pop();
        op2 = unpackI(stack.top()); stack.pop();
        stack.push(pack(op1*op2));
      }
        break;

      case BC_IDIV: {
        int64_t op1, op2;
        
        op1 = unpackI(stack.top()); stack.pop();
        op2 = unpackI(stack.top()); stack.pop();
        stack.push(pack(op1/op2));
      }
        break;

        
      case BC_DADD: {
        double op1, op2;

        op1 = unpackD(stack.top()); stack.pop();
        op2 = unpackD(stack.top()); stack.pop();
        stack.push(pack(op1 + op2));
      }
        break;

      case BC_DSUB: {
        double op1, op2;

        op1 = unpackD(stack.top()); stack.pop();
        op2 = unpackD(stack.top()); stack.pop();
        stack.push(pack(op1 - op2));
      }
        break;

      case BC_DMUL: {
        double op1, op2;

        op1 = unpackD(stack.top()); stack.pop();
        op2 = unpackD(stack.top()); stack.pop();
        stack.push(pack(op1*op2));
      }
        break;

      case BC_DDIV: {
        double op1, op2;

        op1 = unpackD(stack.top()); stack.pop();
        op2 = unpackD(stack.top()); stack.pop();
        stack.push(pack(op1/op2));
      }
        break;

      case BC_DNEG: {
        double op;

        op = unpackD(stack.top()); stack.pop();
        stack.push(pack(-op));
      }
        break;

        // Conversion

      case BC_I2D: {
        int64_t op;

        op = unpackI(stack.top()); stack.pop();
        stack.push(pack((double)op));
      }

      case BC_D2I: {
        double op;

        op = unpackI(stack.top()); stack.pop();
        stack.push(pack((int64_t)op));
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

        id = code.get(ip + 1);
        stack.push(var_pool[id]);
      }
        break;

      case BC_STOREDVAR:
      case BC_STOREIVAR: {
        int8_t id;

        id = code.get(ip + 1);
        var_pool[id] = stack.top(); stack.pop();
      }
        break;

        // Comparision

      case BC_DCMP: {
        double op1, op2;

        op1 = unpackD(stack.top()); stack.pop();
        op2 = unpackD(stack.top()); stack.pop();
        
        if (op1 > op2) {
          stack.push(pack(1));
        } else if (op1 == op2) {
          stack.push(pack(0));
        } else {
          stack.push(pack(-1));
        }
      }
        break;

      case BC_ICMP: {
        int64_t op1, op2;

        op1 = unpackI(stack.top()); stack.pop();
        op2 = unpackI(stack.top()); stack.pop();
        
        if (op1 > op2) {
          stack.push(pack(1));
        } else if (op1 == op2) {
          stack.push(pack(0));
        } else {
          stack.push(pack(-1));
        }
      }
        break;

        // Jumps

      case BC_JA: {
        int32_t offset = code.getTyped<int32_t>(ip + 1);
        ip += offset;        
      }
        break;

      case BC_IFICMPNE: {
        int64_t op1, op2;
        int32_t offset;

        op1 = unpackI(stack.top()); stack.pop();
        op2 = unpackI(stack.top()); stack.pop();

        if (op1 != op2) {        
          offset = code.getTyped<int32_t>(ip + 1);
          ip += offset;
        }
      }
        break;

      case BC_IFICMPE: {
        int64_t op1, op2;
        int32_t offset;

        op1 = unpackI(stack.top()); stack.pop();
        op2 = unpackI(stack.top()); stack.pop();

        if (op1 == op2) {        
          offset = code.getTyped<int32_t>(ip + 1);
          ip += offset;
        }
      }
        break;

      case BC_IFICMPG: {
        int64_t op1, op2;
        int32_t offset;

        op1 = unpackI(stack.top()); stack.pop();
        op2 = unpackI(stack.top()); stack.pop();

        if (op1 > op2) {        
          offset = code.getTyped<int32_t>(ip + 1);
          ip += offset;
        }
      }
        break;

      case BC_IFICMPGE: {
        int64_t op1, op2;
        int32_t offset;

        op1 = unpackI(stack.top()); stack.pop();
        op2 = unpackI(stack.top()); stack.pop();

        if (op1 != op2) {        
          offset = code.getTyped<int32_t>(ip + 1);
          ip += offset;
        }
      }
        break;

      case BC_IFICMPL: {
        int64_t op1, op2;
        int32_t offset;

        op1 = unpackI(stack.top()); stack.pop();
        op2 = unpackI(stack.top()); stack.pop();

        if (op1 < op2) {        
          offset = code.getTyped<int32_t>(ip + 1);
          ip += offset;
        }
      }
        break;

      case BC_IFICMPLE: {
        int64_t op1, op2;
        int32_t offset;

        op1 = unpackI(stack.top()); stack.pop();
        op2 = unpackI(stack.top()); stack.pop();

        if (op1 <= op2) {        
          offset = code.getTyped<int32_t>(ip + 1);
          ip += offset;
        }
      }
        break;

      default:
        break;
      }

      ip += opcode_len[opcode];
    }

    return 0;
  }
};
