//
// Created by wimag on 23.11.16.
//

#include "bytecode_executor.h"
using namespace mathvm;

bytecode_executor::bytecode_executor(InterpreterCodeImpl *code): code(code), storage(code) {}

Status* bytecode_executor::execute() {
    storage.enter_function((BytecodeFunction *) code->functionByName("<top>"));

    while(!storage.finished()){
        execute_instruction(storage.next_instruction());
    }
    return Status::Ok();
}

void bytecode_executor::execute_instruction(Instruction insn) {
    stack_var v1, v2, v3; //tmp vals
    uint16_t addr1, addr2;
    int64_t i1, i2;
    TranslatedFunction* function;
    switch(insn){

        case BC_INVALID: // INVALID OP
            assert(false);
            break;
        case BC_DLOAD: //LOADS
            storage.push_stack(storage.load_double());
            break;
        case BC_ILOAD:
            storage.push_stack(storage.load_int());
            break;
        case BC_SLOAD:
            storage.push_stack(storage.load_string());
            break;
        case BC_DLOAD0: //CONSTANT LOADS
            storage.push_stack(0.0);
            break;
        case BC_ILOAD0:
            storage.push_stack((int64_t)0);
            break;
        case BC_SLOAD0:
            storage.push_stack("");
            break;
        case BC_DLOAD1:
            storage.push_stack(1.0);
            break;
        case BC_ILOAD1:
            storage.push_stack((int64_t )1);
            break;
        case BC_DLOADM1:
            storage.push_stack(-1.0);
            break;
        case BC_ILOADM1:
            storage.push_stack((int64_t )-1);
            break;
        case BC_DADD: // DOUBLE BIN OPS
        case BC_DSUB:
        case BC_DMUL:
        case BC_DDIV:
            execute_double_binop(insn);
            break;
        case BC_IADD: // INT BIN OPS
        case BC_ISUB:
        case BC_IMUL:
        case BC_IDIV:
        case BC_IMOD:
        case BC_IAAND:
        case BC_IAOR:
        case BC_IAXOR:
            execute_int_binop(insn);
            break;
        case BC_DNEG:
            v1 = storage.pop_stack();
            storage.push_stack(-(double)v1);
            break;
        case BC_INEG:
            v1 = storage.pop_stack();
            storage.push_stack(-(int64_t)v1);
            break;
        case BC_IPRINT: // Prints
        case BC_DPRINT:
        case BC_SPRINT:
            std::cout << storage.pop_stack();
            break;
        case BC_I2D:
            v1 = storage.pop_stack();
            storage.push_stack((double)((int64_t) v1));
            break;
        case BC_D2I:
            v1 = storage.pop_stack();
            storage.push_stack((int64_t)((double) v1));
            break;
        case BC_S2I:
            v1 = storage.pop_stack();
            assert(v1.type() == VT_STRING);
            storage.push_stack(reinterpret_cast<int64_t >((const char *)v1));
            break;
        case BC_SWAP:
            v1 = storage.pop_stack();
            v2 = storage.pop_stack();
            storage.push_stack(v1);
            storage.push_stack(v2);
            break;
        case BC_POP:
            storage.pop_stack();
            break;
        case BC_LOADDVAR0: // LOAD first var
        case BC_LOADIVAR0:
        case BC_LOADSVAR0:
            storage.push_stack(storage.get_local_var(0));
            break;
        case BC_LOADDVAR1:
        case BC_LOADIVAR1:
        case BC_LOADSVAR1:
            storage.push_stack(storage.get_local_var(1));
            break;
        case BC_LOADDVAR2:
        case BC_LOADIVAR2:
        case BC_LOADSVAR2:
            storage.push_stack(storage.get_local_var(2));
            break;
        case BC_LOADDVAR3:
        case BC_LOADIVAR3:
        case BC_LOADSVAR3:
            storage.push_stack(storage.get_local_var(0));
            break;
        case BC_STOREDVAR0:
        case BC_STOREIVAR0:
        case BC_STORESVAR0:
            v1 = storage.pop_stack();
            storage.set_local_var(v1, 0);
            break;
        case BC_STOREDVAR1:
        case BC_STOREIVAR1:
        case BC_STORESVAR1:
            v1 = storage.pop_stack();
            storage.set_local_var(v1, 1);
            break;
        case BC_STOREDVAR2:
        case BC_STOREIVAR2:
        case BC_STORESVAR2:
            v1 = storage.pop_stack();
            storage.set_local_var(v1, 2);
            break;
        case BC_STOREDVAR3:
        case BC_STOREIVAR3:
        case BC_STORESVAR3:
            v1 = storage.pop_stack();
            storage.set_local_var(v1, 3);
            break;
        case BC_LOADDVAR:
        case BC_LOADIVAR:
        case BC_LOADSVAR:
            addr1 = storage.load_addr();
            v1 = storage.get_local_var(addr1);
            storage.push_stack(v1);
            break;
        case BC_STOREDVAR:
        case BC_STOREIVAR:
        case BC_STORESVAR:
            addr1 = storage.load_addr();
            v1 = storage.pop_stack();
            storage.set_local_var(v1, addr1);
            break;
        case BC_LOADCTXDVAR:
        case BC_LOADCTXIVAR:
        case BC_LOADCTXSVAR:
            addr1 = storage.load_addr();
            addr2 = storage.load_addr();
            v1 = storage.get_context_var(addr1, addr2);
            storage.push_stack(v1);
            break;
        case BC_STORECTXDVAR:
        case BC_STORECTXIVAR:
        case BC_STORECTXSVAR:
            addr1 = storage.load_addr();
            addr2 = storage.load_addr();
            v1 = storage.pop_stack();
            storage.set_context_var(v1, addr1, addr2);
            break;
        case BC_DCMP:
            v1 = storage.pop_stack();
            v2 = storage.pop_stack();
            i1 = 0;
            if((double) v1 != (double) v2){
                i1 = (double) v1 < (double) v2 ? -1 : 1;
            }
            storage.push_stack(i1);
            break;
        case BC_ICMP:
            v1 = storage.pop_stack();
            v2 = storage.pop_stack();
            i1 = 0;
            if((int64_t) v1 != (int64_t) v2){
                i1 = (int64_t) v1 < (int64_t) v2 ? -1 : 1;
            }
            storage.push_stack(i1);
            break;
        case BC_JA:
            addr1 = storage.load_addr();
            storage.jump(addr1 - sizeof(uint16_t));
            break;
        case BC_IFICMPNE:
        case BC_IFICMPE:
        case BC_IFICMPG:
        case BC_IFICMPGE:
        case BC_IFICMPL:
        case BC_IFICMPLE:
            execute_conditional_jump(insn);
            break;
        case BC_DUMP:
            v1 = storage.pop_stack();
            std::cerr << v1;
            storage.push_stack(v1);
        case BC_BREAK:break;
        case BC_STOP:
            cerr << "Execution halted by STOP instruction";
            exit(1);
        case BC_CALL:
            addr1 = storage.load_addr();
            function = code->functionById(addr1);
            assert(function != nullptr);
            storage.enter_function((BytecodeFunction *) function);
            break;
        case BC_CALLNATIVE:break;
        case BC_RETURN:
            storage.exit_function();
            break;
        case BC_LAST:break;
    }
}

void bytecode_executor::execute_int_binop(Instruction insn) {
    stack_var v1 = storage.pop_stack();
    stack_var v2 = storage.pop_stack();
    int64_t res;
    switch (insn){
        case BC_IADD:
            res = (int64_t)v1 + (int64_t)v2;
            break;
        case BC_ISUB:
            res = (int64_t)v1 - (int64_t)v2;
            break;
        case BC_IMUL:
            res = (int64_t)v1 * (int64_t)v2;
            break;
        case BC_IDIV:
            res = (int64_t)v1 / (int64_t)v2;
            break;
        case BC_IMOD:
            res = (int64_t)v1 % (int64_t)v2;
            break;
        case BC_IAOR:
            res = (int64_t)v1 | (int64_t)v2;
            break;
        case BC_IAAND:
            res = (int64_t)v1 & (int64_t)v2;
            break;
        case BC_IAXOR:
            res = (int64_t)v1 ^ (int64_t)v2;
            break;
        default:
            assert(false);
            break;
    }
    storage.push_stack(res);
}

void bytecode_executor::execute_double_binop(Instruction insn) {
    stack_var v1 = storage.pop_stack();
    stack_var v2 = storage.pop_stack();
    double res;
    switch (insn){
        case BC_DADD:
            res = (double)v1 + (double)v2;
            break;
        case BC_DSUB:
            res = (double)v1 - (double)v2;
            break;
        case BC_DMUL:
            res = (double)v1 * (double)v2;
            break;
        case BC_DDIV:
            res = (double)v1 / (double)v2;
            break;
        default:
            assert(false);
            break;
    }
    storage.push_stack(res);
}

void bytecode_executor::execute_conditional_jump(Instruction insn) {
    stack_var v1 = storage.pop_stack();
    stack_var v2 = storage.pop_stack();
    int64_t a = (int64_t)v1;
    int64_t b = (int64_t)v2;
    int16_t offset = (int16_t)storage.load_addr() - sizeof(uint16_t);
    bool flag;
    switch (insn){
        case BC_IFICMPNE:
            flag = a != b;
            break;
        case BC_IFICMPE:
            flag = a == b;
            break;
        case BC_IFICMPG:
            flag = a > b;
            break;
        case BC_IFICMPGE:
            flag = a >= b;
            break;
        case BC_IFICMPL:
            flag = a < b;
            break;
        case BC_IFICMPLE:
            flag = a <= b;
            break;
        default:
            assert(false);
    }
    if(flag){
        storage.jump(offset);
    }

}
