#include "stack_machine.h"

namespace mathvm {

size_t functionsCount(Code * code) {
    size_t result = 0;
    Code::FunctionIterator it(code);
    while (it.hasNext()) {
        ++result;
        it.next();
    }

    return result;
}

void StackMachine::execute(Code * code) {
    code_ = code;
    data_stack_.reserve(max_stack_size_);
    return_addresses_.reserve(max_stack_size_);
    stack_frame_starts_.reserve(max_stack_size_);

    stack_top_ptr_ = data_stack_.data();
    return_addresses_top_ptr_ = return_addresses_.data();
    stack_frame_starts_top_ptr_ = stack_frame_starts_.data();
    
    current_stack_frame_start_ = stack_top_ptr_;

    frame_start_of_last_function_call_.reserve(functionsCount(code));
    current_function_ = nullptr;

    processCall(0);

    run();
}

template<typename T> 
vm_int_t cmp(T a, T b) {
    if (a == b) return 0L;
    return a > b ? 1L : -1L;
}

static const char * __EMPTY_STRING = "";

void StackMachine::run() {
    while (true) {
        Instruction current_instruction = currentBytecode().getInsn(current_location_++);
        char const * v  = 0;
        switch (current_instruction) {
            case BC_DLOAD:
                push(currentBytecode().getDouble(current_location_));
                current_location_ += sizeof(double);
                break;
            case BC_DLOAD0:
                push(0.0);
                break;
            case BC_DLOAD1:
                push(1.0);
                break;
            case BC_ILOAD:
                push(currentBytecode().getInt64(current_location_));
                current_location_ += sizeof(vm_int_t);
                break;
            case BC_ILOAD0:
                push(0L);
                break;
            case BC_ILOAD1:
                push(1L);
                break;
            case BC_SLOAD:
                push(code_->constantById(getCurrent2BytesAndShiftLocation()).c_str());
                break;
            case BC_SLOAD0:
                push(__EMPTY_STRING);
                break;
            case BC_DADD:
                PUSH_BINARY_RESULT(double, std::plus<double>());
                break;
            case BC_DSUB:
                PUSH_BINARY_RESULT(double, std::minus<double>());
                break;
            case BC_DMUL:
                PUSH_BINARY_RESULT(double, std::multiplies<double>());
                break;
            case BC_DDIV:
                PUSH_BINARY_RESULT(double, std::divides<double>());
                break;
            case BC_IADD:
                PUSH_BINARY_RESULT(vm_int_t, std::plus<vm_int_t>());
                break;
            case BC_ISUB:
                PUSH_BINARY_RESULT(vm_int_t, std::minus<vm_int_t>());
                break;
            case BC_IMUL:
                PUSH_BINARY_RESULT(vm_int_t, std::multiplies<vm_int_t>());
                break;
            case BC_IDIV:
                PUSH_BINARY_RESULT(vm_int_t, std::divides<vm_int_t>());
                break;
            case BC_IMOD:
                PUSH_BINARY_RESULT(vm_int_t, std::modulus<vm_int_t>());
                break;
            case BC_IAOR:
                PUSH_BINARY_RESULT(vm_int_t, std::bit_or<vm_int_t>());
                break;
            case BC_IAAND:
                PUSH_BINARY_RESULT(vm_int_t, std::bit_and<vm_int_t>());
                break;
            case BC_IAXOR:
                PUSH_BINARY_RESULT(vm_int_t, std::bit_xor<vm_int_t>());
                break;
            case BC_DCMP:
                PUSH_BINARY_RESULT_(double, vm_int_t, cmp<double>);
                break;
            case BC_ICMP:
                PUSH_BINARY_RESULT(vm_int_t, cmp<vm_int_t>);
                break;
            case BC_DNEG:
                push(-popDouble());
                break;
            case BC_INEG:
                push(-popInt());
                break;
            case BC_IPRINT:
                os_ << popInt();
                break;
            case BC_DPRINT:
                os_ << popDouble();
                break;
            case BC_SPRINT:
                v = popString();
                os_ << (v == 0 ? "" : v);
                break;
            case BC_I2D:
                push((double) popInt());
                break;
            case BC_S2I:
                v = popString();
                try {
                    push(v == 0 ? 0L : (vm_int_t)stoll(v));
                } catch (std::exception & e) {
                    throwError("S2I conversion error: " + string(v));
                }
                break;
            case BC_D2I:
                push((vm_int_t) popDouble());
                break;
            case BC_POP:
                pop();
                break;
            case BC_LOADDVAR0:
                loadLocalVar<double>(0);
                break;
            case BC_LOADDVAR1:
                loadLocalVar<double>(1);
                break;
            case BC_LOADDVAR2:
                loadLocalVar<double>(2);
                break;
            case BC_LOADDVAR3:
                loadLocalVar<double>(3);
                break;
            case BC_LOADDVAR:
                loadLocalVar<double>(getCurrent2BytesAndShiftLocation());
                break;
            case BC_LOADIVAR0:
                loadLocalVar<vm_int_t>(0);
                break;
            case BC_LOADIVAR1:
                loadLocalVar<vm_int_t>(1);
                break;
            case BC_LOADIVAR2:
                loadLocalVar<vm_int_t>(2);
                break;
            case BC_LOADIVAR3:
                loadLocalVar<vm_int_t>(3);
                break;
            case BC_LOADIVAR:
                loadLocalVar<vm_int_t>(getCurrent2BytesAndShiftLocation());
                break;
            case BC_LOADSVAR0:
                loadLocalVar<vm_str_t>(0);
                break;
            case BC_LOADSVAR1:
                loadLocalVar<vm_str_t>(1);
                break;
            case BC_LOADSVAR2:
                loadLocalVar<vm_str_t>(2);
                break;
            case BC_LOADSVAR3:
                loadLocalVar<vm_str_t>(3);
                break;
            case BC_LOADSVAR:
                loadLocalVar<vm_str_t>(getCurrent2BytesAndShiftLocation());
                break;
            case BC_STOREDVAR0:
                storeLocalVar<double>(0);
                break;
            case BC_STOREDVAR1:
                storeLocalVar<double>(1);
                break;
            case BC_STOREDVAR2:
                storeLocalVar<double>(2);
                break;
            case BC_STOREDVAR3:
                storeLocalVar<double>(3);
                break;
            case BC_STOREDVAR:
                storeLocalVar<double>(getCurrent2BytesAndShiftLocation());
                break;
            case BC_STOREIVAR0:
                storeLocalVar<vm_int_t>(0);
                break;
            case BC_STOREIVAR1:
                storeLocalVar<vm_int_t>(1);
                break;
            case BC_STOREIVAR2:
                storeLocalVar<vm_int_t>(2);
                break;
            case BC_STOREIVAR3:
                storeLocalVar<vm_int_t>(3);
                break;
            case BC_STOREIVAR:
                storeLocalVar<vm_int_t>(getCurrent2BytesAndShiftLocation());
                break;
            case BC_STORESVAR0:
                storeLocalVar<vm_str_t>(0);
                break;
            case BC_STORESVAR1:
                storeLocalVar<vm_str_t>(1);
                break;
            case BC_STORESVAR2:
                storeLocalVar<vm_str_t>(2);
                break;
            case BC_STORESVAR3:
                storeLocalVar<vm_str_t>(3);
                break;
            case BC_STORESVAR:
                storeLocalVar<vm_str_t>(getCurrent2BytesAndShiftLocation());
                break;
            case BC_LOADCTXDVAR:
                processLoadContextVar<double>();
                break;
            case BC_LOADCTXIVAR:
                processLoadContextVar<vm_int_t>();
                break;
            case BC_LOADCTXSVAR:
                processLoadContextVar<vm_str_t>();
                break;
            case BC_STORECTXDVAR:
                processStoreContextVar<double>();
                break;
            case BC_STORECTXIVAR:
                processStoreContextVar<vm_int_t>();
                break;
            case BC_STORECTXSVAR:
                processStoreContextVar<vm_str_t>();
                break;
            case BC_JA:
                current_location_ = calculateTransitionAndShiftLocation();
                continue;
            case BC_IFICMPE:
            case BC_IFICMPNE:
            case BC_IFICMPL:
            case BC_IFICMPLE:
            case BC_IFICMPG:
            case BC_IFICMPGE: {
                    index_t transition = calculateTransitionAndShiftLocation();

                    int a = popInt();
                    int b = popInt();

                    if (ifSatisfied(current_instruction, cmp(a, b))) {
                        current_location_= transition;
                        continue;
                    }
                    break;
                }
            case BC_CALL:
                processCall(getCurrent2BytesAndShiftLocation());
                break;
            case BC_CALLNATIVE:
                processNativeCall(getCurrent2BytesAndShiftLocation());
                continue;
            case BC_RETURN:
                if (processReturn()) {
                    return;
                }
                continue;
            default:
                throwError("unsupported insn=" + string(bytecodeName(current_instruction)));
                return;
        }
    }
}

bool StackMachine::ifSatisfied(Instruction condition, vm_int_t value) {
    switch(condition) {
        case BC_IFICMPE:
            return value == 0L;
        case BC_IFICMPNE:
            return value != 0L;
        case BC_IFICMPL:
            return value < 0L;
        case BC_IFICMPLE:
            return value <= 0L;
        case BC_IFICMPG:
            return value > 0L;
        case BC_IFICMPGE:
            return value >= 0L;
        default:
            throwError("unsupported condition=" + string(bytecodeName(condition)));
            return false;
    }
}

void StackMachine::processCall(index_t id) {
    if (current_function_ != 0) {
        *(return_addresses_top_ptr_++) = {current_function_->id(), current_location_, frame_start_of_last_function_call_[id]};
    }

    current_function_ = static_cast<BytecodeFunction*>(code_->functionById(id));
    current_location_ = 0;

    StackValue * callee_stack_start = stack_top_ptr_ - current_function_->parametersNumber();
    
    current_stack_frame_start_ = callee_stack_start;
    *(stack_frame_starts_top_ptr_++) = current_stack_frame_start_;

    stack_top_ptr_ += current_function_->localsNumber();

    frame_start_of_last_function_call_[id] = callee_stack_start;  
}

bool StackMachine::processReturn() {
    if (return_addresses_top_ptr_ == return_addresses_.data()) return true;
    
    StackValue * new_stack_top = *(--stack_frame_starts_top_ptr_);
    current_stack_frame_start_ = *(stack_frame_starts_top_ptr_ - 1);
    
    if (current_function_->returnType() != VT_VOID) {
        StackValue return_value = *(--stack_top_ptr_);
        *(new_stack_top++) = return_value;
    }
    
    stack_top_ptr_ = new_stack_top;

    CodePointer return_address = *(--return_addresses_top_ptr_);

    frame_start_of_last_function_call_[current_function_->id()] = return_address.previous_call_of_same_function_start_;

    current_function_ = static_cast<BytecodeFunction*>(code_->functionById(return_address.function_id));
    current_location_ = return_address.bytecode_location;

    return false;
}

typedef long long word_t;

word_t make_call(double fargs[8], word_t iargs[6], const void * addr, word_t * args, word_t * last) {
    register word_t* sp  asm("rsp"); 
    
    word_t stack_window = 32;
    word_t stack_delta = stack_window * sizeof(word_t) ; 
    word_t * copy = (sp - stack_window);
    
    while (args != last) {
        *(copy++) = *(args++);
    }
    
    register word_t iret asm("rax");
    asm volatile ("movsd %0, %%xmm7;" : : "m" (fargs[7]) : "xmm7");

    asm volatile ("\
        movq %0, %%r11;\n\n\
        movq %1, %%rdi;\n\n\
        movq %2, %%rsi;\n\
        movq %3, %%rdx;\n\
        movq %4, %%rcx;\n\
        movq %5, %%r8;\n\
        movq %6, %%r9;\n\
        movsd %7, %%xmm0;\n\
        movsd %8, %%xmm1;\n\
        movsd %9, %%xmm2;\n\
        movsd %10, %%xmm3;\n\
        movsd %11, %%xmm4;\n\
        movsd %12, %%xmm5;\n\
        movsd %13, %%xmm6;\n\
        subq %14, %%rsp;\n\
        call *%%r11;\n\
        addq %14, %%rsp;\n\
        " 
        : 
        : "g"(addr), "g"(iargs[0]), "g"(iargs[1]),"g"(iargs[2]),"g"(iargs[3]),"g"(iargs[4]),"g"(iargs[5]) 
          ,"g"(fargs[0]),"g"(fargs[1]),"g"(fargs[2]),"g"(fargs[3]),"g"(fargs[4]),"g"(fargs[5]),"g"(fargs[6])
          ,"g"(stack_delta), "g"(stack_delta) 
        : "r11", "rdi", "edi", "rsi", "esi", "rdx", "edx", "rcx", "ecx", "r8", "r9", 
          "xmm0","xmm1","xmm2","xmm3","xmm4","xmm5","xmm6","xmm7", "rsp"
    );

    return iret + 0*stack_delta;
}

void StackMachine::processNativeCall(index_t id) {
    Signature const * signature;
    string const * name;
    
    const void * addr = code_->nativeById(id, &signature, &name);
    
    word_t iargs[6];
    double fargs[8];
    
    word_t * iarg = iargs;
    double * farg = fargs;
    
    StackValue * p = stack_top_ptr_ - signature->size();

    word_t args[256];
    word_t *argframe = args;
    
    for (size_t i = 1; i < signature->size(); ++i) {
        StackValue * value = p + i;
        
        switch((*signature)[i].first) {
            case VT_DOUBLE:
            {
                double v = value->doubleValue();
                if (fargs + 8 > farg) {
                    *(farg++) = v;
                } else {
                    *((double *)(argframe++)) = v;
                }

                break;
            }
            case VT_STRING:
            case VT_INT:
            {
                long long v = value->wordValue();
                if (iargs + 6 > iarg) {
                    *(iarg++) = v;
                } else {
                    *(argframe++) = v;
                }
                break;
            }
            default: 
                throwError("unexpected type in native");
                return;
        }
    }
    
    word_t res = make_call(fargs, iargs, addr, args, argframe);
    auto kind = (*signature)[0].first;
    
    StackValue result;

    switch(kind) {
        case VT_DOUBLE:
        {
            register double dret asm("xmm0");
            result = double(dret);
            break;
        }
        case VT_INT:
        case VT_STRING:
        case VT_VOID:
        {
            result = vm_int_t(res);
            break;
        }
        default:
            throwError("invalid type in native");
            return;
    }
    
    for (size_t i = 1; i < signature->size(); ++i) {
        pop();
    }

    if (signature->at(0).first != VT_VOID) {
        push(result);
    }
}

}
