//
// Created by dsavvinov on 13.11.16.
//

#include <ast.h>
#include "Interpreter.h"
#include "TranslationException.h"

namespace mathvm {

Status * Interpreter::executeProgram() {
    BytecodeFunction * topFunction = (BytecodeFunction *) code->functionByName(AstFunction::top_name);
    executeFunction(topFunction);
}

void Interpreter::executeFunction(BytecodeFunction * function) {
    Bytecode * bc = function->bytecode();
    uint32_t i = 0;
    while (i < bc->length()) {
        Instruction insn = bc->getInsn(i);
        switch(insn) {
            case BC_INVALID: throw ExecutionException("Found BC_INVALID");
            case BC_DLOAD:
                stck.push(new double(bc->getDouble(i + 1)));
                break;
            case BC_ILOAD:
                stck.push(new int(bc->getInt64(i + 1)));
                break;
            case BC_SLOAD:
                string * str = new string(code->constantById((uint16_t) bc->getInt16(i + 1)));
                stck.push(str);
                break;
            case BC_DLOAD0:
                stck.push(new double(0));
                break;
            case BC_ILOAD0:
                stck.push(new int(0));
                break;
            case BC_SLOAD0:
                stck.push(new string());
                break;
            case BC_DLOAD1:
                stck.push(new double(1.0));
                break;
            case BC_ILOAD1:
                stck.push(new int(1));
                break;
            case BC_DLOADM1:
                stck.push(new double(-1.0));
                break;
            case BC_ILOADM1:
                stck.push(new int(-1));
                break;
            case BC_DADD: {
                double * up = (double *) stck.top();
                double * low = (double *) stck.top();
                stck.push(new double(*up + *low));
                delete up;
                delete low;
                break;
            }
            case BC_IADD: {
                int * up = (int *) stck.top();
                int * low = (int *) stck.top();
                stck.push(new int(*up + *low));
                delete up;
                delete low;
                break;
            }
            case BC_DSUB:break;
            case BC_ISUB:break;
            case BC_DMUL:break;
            case BC_IMUL:break;
            case BC_DDIV:break;
            case BC_IDIV:break;
            case BC_IMOD:break;
            case BC_DNEG:break;
            case BC_INEG:break;
            case BC_IAOR:break;
            case BC_IAAND:break;
            case BC_IAXOR:break;
            case BC_IPRINT:break;
            case BC_DPRINT:break;
            case BC_SPRINT:break;
            case BC_I2D:break;
            case BC_D2I:break;
            case BC_S2I:break;
            case BC_SWAP:break;
            case BC_POP:break;
            case BC_LOADDVAR0:break;
            case BC_LOADDVAR1:break;
            case BC_LOADDVAR2:break;
            case BC_LOADDVAR3:break;
            case BC_LOADIVAR0:break;
            case BC_LOADIVAR1:break;
            case BC_LOADIVAR2:break;
            case BC_LOADIVAR3:break;
            case BC_LOADSVAR0:break;
            case BC_LOADSVAR1:break;
            case BC_LOADSVAR2:break;
            case BC_LOADSVAR3:break;
            case BC_STOREDVAR0:break;
            case BC_STOREDVAR1:break;
            case BC_STOREDVAR2:break;
            case BC_STOREDVAR3:break;
            case BC_STOREIVAR0:break;
            case BC_STOREIVAR1:break;
            case BC_STOREIVAR2:break;
            case BC_STOREIVAR3:break;
            case BC_STORESVAR0:break;
            case BC_STORESVAR1:break;
            case BC_STORESVAR2:break;
            case BC_STORESVAR3:break;
            case BC_LOADDVAR:break;
            case BC_LOADIVAR:break;
            case BC_LOADSVAR:break;
            case BC_STOREDVAR:break;
            case BC_STOREIVAR:break;
            case BC_STORESVAR:break;
            case BC_LOADCTXDVAR:break;
            case BC_LOADCTXIVAR:break;
            case BC_LOADCTXSVAR:break;
            case BC_STORECTXDVAR:break;
            case BC_STORECTXIVAR:break;
            case BC_STORECTXSVAR:break;
            case BC_DCMP:break;
            case BC_ICMP:break;
            case BC_JA:break;
            case BC_IFICMPNE:break;
            case BC_IFICMPE:break;
            case BC_IFICMPG:break;
            case BC_IFICMPGE:break;
            case BC_IFICMPL:break;
            case BC_IFICMPLE:break;
            case BC_DUMP:break;
            case BC_STOP:break;
            case BC_CALL:break;
            case BC_CALLNATIVE:break;
            case BC_RETURN:break;
            case BC_BREAK:break;
            case BC_LAST:break;
        }
    }
}

}