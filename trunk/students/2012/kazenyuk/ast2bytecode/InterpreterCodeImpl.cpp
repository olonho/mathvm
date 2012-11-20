#include <iostream>
#include <vector>
#include <string>

#include "InterpreterCodeImpl.h"

namespace mathvm {

inline const char* bcName(Instruction insn, size_t& length) {
    static const struct {
        const char* name;
        Instruction insn;
        size_t length;
    } names[] = {
#define BC_NAME(b, d, l) {#b, BC_##b, l},
        FOR_BYTECODES(BC_NAME)
    };

    if (insn >= BC_INVALID && insn < BC_LAST) {
        length = names[insn].length;
        return names[insn].name;
    }

    assert(false);
    return 0;
}

Status* InterpreterCodeImpl::execute(std::vector<mathvm::Var*>&) {
  BytecodeFunction* function = (BytecodeFunction*) functionById(0);
  Bytecode* bytecode = function->bytecode();

  for (size_t bci = 0; bci < bytecode->length();) {
      size_t length;
      Instruction insn = bytecode->getInsn(bci);
      const char* name = bcName(insn, length);
      std::clog << bci << ": " << name << std::endl;
      switch (insn) {
          case BC_INVALID:
              return new Status(name, bci);
              break;
          case BC_DLOAD:
//                  out << name << " " << getDouble(bci + 1);
              break;
          case BC_ILOAD:
//                  out << name << " " << getInt64(bci + 1);
              // TODO:
              break;
          case BC_SLOAD:
//                  out << name << " @" << getUInt16(bci + 1);
              break;
          case BC_DADD:
              break;
          case BC_IADD:
              // TODO:
              break;
          case BC_IPRINT:
              // TODO:
              break;
          case BC_LOADDVAR:
          case BC_STOREDVAR:
          case BC_LOADIVAR:
          case BC_STOREIVAR:
          case BC_LOADSVAR:
          case BC_STORESVAR:
//                  out << name << " @" << getUInt16(bci + 1);
              break;
//              case BC_LOADCTXDVAR:
//              case BC_STORECTXDVAR:
//              case BC_LOADCTXIVAR:
//              case BC_STORECTXIVAR:
//              case BC_LOADCTXSVAR:
//              case BC_STORECTXSVAR:
////                  out << name << " @" << getUInt16(bci + 1)
////                      << ":" << getUInt16(bci + 3);
//                  break;
          case BC_IFICMPNE:
          case BC_IFICMPE:
          case BC_IFICMPG:
          case BC_IFICMPGE:
          case BC_IFICMPL:
          case BC_IFICMPLE:
          case BC_JA:
//                  out << name << " " << getInt16(bci + 1) + bci + 1;
              bci += bytecode->getInt16(bci + 1) + 1;
              continue;
              break;
          case BC_CALL:
//                  out << name << " *" << getUInt16(bci + 1);
              // TODO: push bci
              // TODO: push functionId
              function = (BytecodeFunction*) functionById(bytecode->getUInt16(bci + 1));
              if (!function) {
                  return new Status("Unresolved function ID\n", bci);
              }
              bytecode = function->bytecode();
              bci = 0;
              continue;
              break;
          case BC_CALLNATIVE:
//                  out << name << " *" << getUInt16(bci + 1);
              return new Status("Native functions are currently not supported\n", bci);
              break;
          case BC_RETURN: {
              uint16_t new_function_id = 0; // TODO: pop functionId
              function = (BytecodeFunction*) functionById(new_function_id);
              if (!function) {
                  return new Status("Unresolved function ID\n", bci);
              }
              bytecode = function->bytecode();
              size_t new_bci = 0;   // TODO: pop bci
              bci = new_bci;
              break;
          }
          case BC_BREAK:
              return new Status("Breakpoints are currently not supported\n", bci);
              break;
          default:
//                  out << name;
              return new Status("Unknown or unsupported instruction\n", bci);
      }
//          out << endl;
      bci += length;
  }
  return 0;
}

}   // namespace mathvm
