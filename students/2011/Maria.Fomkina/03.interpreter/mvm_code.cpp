#include "mathvm.h"
#include "parser.h"
#include "visitors.h"
#include "mvm_code.h"
#include "bytecode_visitor.h"
#include <stack>
#include <vector>
#include <iostream>

namespace mathvm {

union type {
  double d;
  int64_t i;
  uint16_t s;
};

Status* MvmCode::execute(vector<Var*>& vars) {
  
  uint8_t command = 0;
  uint32_t pos = 0;
  uint16_t id;
  int16_t offset;
  std::stack<type> stack;
  type arg; 
  std::vector<double> dvar;
  std::vector<int64_t> ivar;
  std::vector<uint8_t> svar;
  
  while (command != BC_STOP) {
    command = bytecode_->get(pos);
    // std::cerr << "Command " << command << " at " << pos << " stack " 
    //           << stack.size() << std::endl;
    ++pos;
    switch (command) {
      case (BC_DLOAD): {
        arg.d = bytecode_->getDouble(pos);
        stack.push(arg);
        pos += 8;
        break;
      }
      case (BC_ILOAD): {
        arg.i = bytecode_->getInt64(pos);
        stack.push(arg);
        pos += 8;
        break;
      }
      case (BC_SLOAD): {
        arg.s = bytecode_->getInt16(pos);
        stack.push(arg);
        pos += 2;
        break;
      }
      case (BC_DLOAD0): {
        arg.d = 0.0;
        stack.push(arg);
        break;
      }
      case (BC_ILOAD0): {
        arg.i = 0;
        stack.push(arg);
        break;
      }
      case (BC_SLOAD0): {
        arg.s = 0;
        stack.push(arg);
        break;
      }
      case (BC_DLOAD1): {
        arg.d = 1.0;
        stack.push(arg);
        break;
      }
      case (BC_ILOAD1): {
        arg.i = 1;
        stack.push(arg);
        break;
      }
      case (BC_DADD): {
        arg = stack.top();
        stack.pop();
        arg.d += stack.top().d;
        stack.pop();
        stack.push(arg);
        break;
      }
      case (BC_IADD): {
        arg = stack.top();
        stack.pop();
        arg.i += stack.top().i;
        stack.pop();
        stack.push(arg);
        break;
      }
      case (BC_DSUB): {
        arg = stack.top();
        stack.pop();
        arg.d -= stack.top().d;
        stack.pop();
        stack.push(arg);
        break;
      }
      case (BC_ISUB): {
        arg = stack.top();
        stack.pop();
        arg.i -= stack.top().i;
        stack.pop();
        stack.push(arg);
        break;
      }
      case (BC_DMUL): {
        arg = stack.top();
        stack.pop();
        arg.d *= stack.top().d;
        stack.pop();
        stack.push(arg);
        break;
      }
      case (BC_IMUL): {
        arg = stack.top();
        stack.pop();
        arg.i *= stack.top().i;
        stack.pop();
        stack.push(arg);
        break;
      }
      case (BC_DDIV): {
        arg = stack.top();
        stack.pop();
        arg.d /= stack.top().d;
        stack.pop();
        stack.push(arg);
        break;
      }
      case (BC_IDIV): {
        arg = stack.top();
        stack.pop();
        arg.i /= stack.top().i;
        stack.pop();
        stack.push(arg);
        break;
      }
      case (BC_DNEG): {
        arg = stack.top();
        arg.d = -arg.d;
        stack.pop();
        stack.push(arg);
        break;
      }
      case (BC_INEG): {
        arg = stack.top();
        arg.i = -arg.i;
        stack.pop();
        stack.push(arg);
        break;
      }
      case (BC_IPRINT): {
        arg = stack.top();
        printf("%ld", arg.i);
        break;
      }
      case (BC_DPRINT): {
        arg = stack.top();
        printf("%lf", arg.d);
        break;
      }
      case (BC_SPRINT): {
        arg = stack.top();
        std::string s = constantById(arg.s);
        printf("%s", s.c_str());
        break;
      }
      case (BC_I2D): {
        arg = stack.top();
        stack.pop();
        arg.d = (double)arg.i;
        stack.push(arg);
        break;
      }
      case (BC_D2I): {
        arg = stack.top();
        stack.pop();
        arg.i = (uint64_t)arg.d;
        stack.push(arg);
        break;
      }
      case (BC_SWAP): {
        arg = stack.top();
        stack.pop();
        type arg2 = stack.top();
        stack.pop();
        stack.push(arg);
        stack.push(arg2);
        break;
      }
      case (BC_POP): {
        stack.pop();
        break;
      }
      case (BC_LOADDVAR): {
        id = bytecode_->getInt16(pos);
        arg.d = dvar[id];
        stack.push(arg);
        pos += 2;
        break;
      }
      case (BC_LOADIVAR): {
        id = bytecode_->getInt16(pos);
        arg.i = ivar[id];
        stack.push(arg);
        pos += 2;
        break;
      }
      case (BC_LOADSVAR): {
        id = bytecode_->getInt16(pos);
        arg.s = svar[id];
        stack.push(arg);
        pos += 2;
        break;
      }
      case (BC_STOREDVAR): {
        id = bytecode_->getInt16(pos);
        arg = stack.top();
        stack.pop();
        if (id >= dvar.size()) dvar.resize(id + 1);
        dvar[id] = arg.d;
        pos += 2;
        break;
      }
      case (BC_STOREIVAR): {
        id = bytecode_->getInt16(pos);
        arg = stack.top();
        stack.pop();
        if (id >= ivar.size()) ivar.resize(id + 1);
        ivar[id] = arg.i;
        pos += 2;
        break;
      }
      case (BC_STORESVAR): {
        id = bytecode_->getInt16(pos);
        arg = stack.top();
        stack.pop();
        if (id >= svar.size()) svar.resize(id + 1);
        svar[id] = arg.s;
        pos += 2;
        break;
      }
      case (BC_JA): {
        offset = bytecode_->getInt16(pos);
        pos += offset;
        break;
      }
      case (BC_IFICMPNE): {
        offset = bytecode_->getInt16(pos);
        arg = stack.top();
        stack.pop();
        type arg2 = stack.top();
        stack.pop();
        if (arg.i != arg2.i) pos += offset; else pos += 2;
        break;
      }
      case (BC_IFICMPE): {
        offset = bytecode_->getInt16(pos);
        arg = stack.top();
        stack.pop();
        type arg2 = stack.top();
        stack.pop();
        if (arg.i == arg2.i) pos += offset; else pos += 2;
        break;
      }
      case (BC_IFICMPG): {
        offset = bytecode_->getInt16(pos);
        arg = stack.top();
        stack.pop();
        type arg2 = stack.top();
        stack.pop();
        if (arg.i > arg2.i) pos += offset; else pos += 2;
        break;
      }
      case (BC_IFICMPGE): {
        offset = bytecode_->getInt16(pos);
        arg = stack.top();
        stack.pop();
        type arg2 = stack.top();
        stack.pop();
        if (arg.i >= arg2.i) pos += offset; else pos += 2;
        break;
      }
      case (BC_IFICMPL): {
        offset = bytecode_->getInt16(pos);
        arg = stack.top();
        stack.pop();
        type arg2 = stack.top();
        stack.pop();
        if (arg.i < arg2.i) pos += offset; else pos += 2;
        break;
      }
      case (BC_IFICMPLE): {
        offset = bytecode_->getInt16(pos);
        arg = stack.top();
        stack.pop();
        type arg2 = stack.top();
        stack.pop();
        if (arg.i <= arg2.i) pos += offset; else pos += 2;
        break;
      }
      case (BC_STOP): {
        break;
      }
        // BC_CALL
        // BC_RETURN
      default: {
        printf("Unrecognized command!\n");
      }

    }
  }
  
  return new Status();
}

}
