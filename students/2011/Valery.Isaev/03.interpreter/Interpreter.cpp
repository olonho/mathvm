#include <stdio.h>

#include "Interpreter.h"
#include "BytecodeFunction.h"

union VarUnion {
    double _double;
    int64_t _int;
    const char* _string;
};

Interpreter::Interpreter(): Stack(new Value[STACK_SIZE])
    , CallStack(new StackEntry[CALL_STACK_SIZE]) { }

Interpreter::~Interpreter() {
    delete [] Stack;
    delete [] CallStack;
}

mathvm::Status* Interpreter::execute(std::vector<mathvm::Var*>& vars_) {
    disassemble();
    Value tmp;
    CodePtr code;
    Value* stack = Stack;
    Value* vars = stack;
    StackEntry* call_stack = CallStack;
    code.pInsn = static_cast<BytecodeFunction*>
        (functionById(0))->bytecode()->bytecode();
    
    using namespace mathvm;
    while (1) switch (*code.pInsn++) {
    case BC_DLOAD: stack++->vDouble = *code.pDouble++; break;
    case BC_ILOAD: stack++->vInt = *code.pInt++; break;
    case BC_SLOAD: stack++->vStr = constantById(*code.pVar++).c_str(); break;
    case BC_DLOAD0: stack++->vDouble = 0; break;
    case BC_ILOAD0: stack++->vInt = 0; break;
    case BC_SLOAD0: stack++->vStr = ""; break;
    case BC_DLOAD1: stack++->vDouble = 1; break;
    case BC_ILOAD1: stack++->vInt = 1; break;
    case BC_DLOADM1: stack++->vDouble = -1; break;
    case BC_ILOADM1: stack++->vInt = -1; break;
    case BC_DNEG: (stack - 1)->vDouble *= -1; break;
    case BC_INEG: (stack - 1)->vInt *= -1; break;
    case BC_DPRINT: printf("%lf", (--stack)->vDouble); break;
    case BC_IPRINT: printf("%ld", (--stack)->vInt); break;
    case BC_SPRINT: printf("%s", (--stack)->vStr); break;
    case BC_I2D: (stack - 1)->vDouble = (stack - 1)->vInt; break;
    case BC_POP: --stack; break;
    case BC_STOP: return 0;
    case BC_JA: code.pInsn += *code.pJmp; break;
    case BC_IFICMPNE: stack->vInt -= 2; if ((stack + 1)->vInt != stack->vInt)
                        code.pInsn += *code.pJmp; else code.pInsn += 2; break;
    case BC_IFICMPE : stack->vInt -= 2; if ((stack + 1)->vInt == stack->vInt)
                        code.pInsn += *code.pJmp; else code.pInsn += 2; break;
    case BC_IFICMPG : stack->vInt -= 2; if ((stack + 1)->vInt > stack->vInt)
                        code.pInsn += *code.pJmp; else code.pInsn += 2; break;
    case BC_IFICMPGE: stack->vInt -= 2; if ((stack + 1)->vInt >= stack->vInt)
                        code.pInsn += *code.pJmp; else code.pInsn += 2; break;
    case BC_IFICMPL : stack->vInt -= 2; if ((stack + 1)->vInt < stack->vInt)
                        code.pInsn += *code.pJmp; else code.pInsn += 2; break;
    case BC_IFICMPLE: stack->vInt -= 2; if ((stack + 1)->vInt <= stack->vInt)
                        code.pInsn += *code.pJmp; else code.pInsn += 2; break;
    case BC_DADD: { tmp.vDouble = (--stack)->vDouble;
                    (stack - 1)->vDouble += tmp.vDouble;
                  } break;
    case BC_IADD: { tmp.vInt = (--stack)->vInt;
                    (stack - 1)->vInt += tmp.vInt;
                  } break;
    case BC_DSUB: { tmp.vDouble = (--stack)->vDouble;
                    (stack - 1)->vDouble = tmp.vDouble - (stack - 1)->vDouble;
                  } break;
    case BC_ISUB: { tmp.vInt = (--stack)->vInt;
                    (stack - 1)->vInt = tmp.vInt - (stack - 1)->vInt;
                  } break;
    case BC_DMUL: { tmp.vDouble = (--stack)->vDouble;
                    (stack - 1)->vDouble *= tmp.vDouble;
                  } break;
    case BC_IMUL: { tmp.vInt = (--stack)->vInt;
                    (stack - 1)->vInt *= tmp.vInt;
                  } break;
    case BC_DDIV: { tmp.vDouble = (--stack)->vDouble;
                    (stack - 1)->vDouble = tmp.vDouble / (stack - 1)->vDouble;
                  } break;
    case BC_IDIV: { tmp.vInt = (--stack)->vInt;
                    (stack - 1)->vInt = tmp.vInt / (stack - 1)->vInt;
                  } break;
    case BC_SWAP: { tmp = *(stack - 2);
                    *(stack - 2) = *(stack - 1);
                    *(stack - 1) = tmp;
                  } break;
    case BC_DCMP: { tmp.vDouble = (--stack)->vDouble;
                    (stack - 1)->vInt = tmp.vDouble < (stack - 1)->vDouble ? 1
                        : tmp.vDouble == (stack -1 )->vDouble ? 0 : -1;
                  } break;
    case BC_LOADDVAR: case BC_LOADIVAR: case BC_LOADSVAR:
                    *stack++ = vars[*code.pVar++]; break;
    case BC_STOREDVAR0: case BC_STOREIVAR0: case BC_STORESVAR0:
                    *vars = *--stack; break;
    case BC_STOREDVAR: case BC_STOREIVAR: case BC_STORESVAR:
                    vars[*code.pVar++] = *--stack; break;
    case BC_CALL: { call_stack->code_ptr = code;
                    call_stack->vars_ptr = vars;
                    ++call_stack;
                    ::BytecodeFunction* fun = static_cast< ::BytecodeFunction*>
                        (functionById(*code.pVar++));
                    vars = stack - (fun->parametersNumber() + fun->freeVars() + 1);
                    code.pInsn = fun->bytecode()->bytecode();
                    break;
                  }
    case BC_RETURN: --call_stack;
                    code = call_stack->code_ptr;
                    vars = call_stack->vars_ptr;
                    break;
    }
}
