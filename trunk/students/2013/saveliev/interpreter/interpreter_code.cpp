#include "interpreter_code.h"

namespace mathvm {

using std::cerr;
using std::cout;
using std::endl;
using std::string;


Status* InterpreterCodeImpl::execute(std::vector<Var*>& vars) { 
    INFO("\nEXECUTION");
    
    call(0);
      
    while (true) {
        Instruction insn = _bc->getInsn(_bci);
#if DEB
        printInsn(_bc, _bci);
#endif
        _bci += 1;

        switch (insn) {
            case BC_INVALID: return new Status("Invalid instruction."); break;

            case BC_DLOAD: pushDouble(readDouble()); break;                
            case BC_ILOAD: pushInt64(readInt64()); break;                
            case BC_SLOAD: pushUInt16(readUInt16()); break;
                
            case BC_DLOAD0: pushDouble(0.0); break;                
            case BC_ILOAD0: pushInt64(0); break;            
            case BC_SLOAD0: break;            
            case BC_DLOAD1: pushDouble(1.0); break;            
            case BC_ILOAD1: pushInt64(1); break;            
            case BC_DLOADM1: pushDouble(-1.0); break;            
            case BC_ILOADM1: pushInt64(-1); break;
                
#           define BIN_D(op) { \
                double top = popDouble(); \
                pushDouble(top op popDouble()); \
            }
#           define BIN_I(op) { \
                int top = popInt64(); \
                pushInt64(top op popInt64()); \
            }  
            case BC_DADD: BIN_D(+); break;
            case BC_IADD: BIN_I(+); break;
            case BC_DSUB: BIN_D(-); break;
            case BC_ISUB: BIN_I(-); break;
            case BC_DMUL: BIN_D(*); break;
            case BC_IMUL: BIN_I(*); break;
            case BC_DDIV: BIN_D(/); break;
            case BC_IDIV: BIN_I(/); break;
            case BC_IMOD: BIN_I(%); break;
            
            case BC_DNEG: pushDouble(-popDouble()); break;
            case BC_INEG: pushInt64(-popInt64()); break;
            
            case BC_IAOR: BIN_I(|); break;
            case BC_IAAND: BIN_I(&); break;
            case BC_IAXOR: BIN_I(^); break;

            case BC_IPRINT: cout << popInt64(); break;
            case BC_DPRINT: cout << popDouble(); break;            
            case BC_SPRINT: cout << constantById(popUInt16()); break;

            case BC_I2D: pushDouble((double) popInt64()); break;
            case BC_D2I: pushInt64((int64_t) popDouble()); break;
            case BC_S2I: pushInt64((int64_t) popUInt16()); break;
            
            case BC_SWAP: {
                Val fst = pop();
                Val snd = pop();
                push(fst);
                push(snd);
                break;
            }
            case BC_POP: pop(); break;

            case BC_LOADDVAR0:
            case BC_LOADIVAR0:
            case BC_LOADSVAR0: loadVar(0); break;
            case BC_LOADDVAR1:
            case BC_LOADIVAR1:
            case BC_LOADSVAR1: loadVar(1); break;
            case BC_LOADDVAR2:
            case BC_LOADIVAR2:
            case BC_LOADSVAR2: loadVar(2); break;
            case BC_LOADDVAR3:
            case BC_LOADIVAR3:
            case BC_LOADSVAR3: loadVar(3); break;

            case BC_STOREDVAR0:
            case BC_STOREIVAR0:
            case BC_STORESVAR0: storeVar(0); break;
            case BC_STOREDVAR1:
            case BC_STOREIVAR1:
            case BC_STORESVAR1: storeVar(1); break;
            case BC_STOREDVAR2:
            case BC_STOREIVAR2:
            case BC_STORESVAR2: storeVar(2); break;
            case BC_STOREDVAR3:
            case BC_STOREIVAR3:
            case BC_STORESVAR3: storeVar(3); break;
                        
            case BC_LOADDVAR:
            case BC_LOADIVAR:
            case BC_LOADSVAR: loadVar(readUInt16()); break;
            
            case BC_STOREDVAR:
            case BC_STOREIVAR:
            case BC_STORESVAR: storeVar(readUInt16()); break;

            case BC_LOADCTXDVAR: 
            case BC_LOADCTXIVAR: 
            case BC_LOADCTXSVAR:
            case BC_STORECTXDVAR:
            case BC_STORECTXIVAR: 
            case BC_STORECTXSVAR: {
                uint16_t ctxId = readUInt16();
                Context* ctx = findContext(ctxId);
                assert(ctx);
                
                switch (insn) {
                    case BC_LOADCTXDVAR: case BC_LOADCTXIVAR: case BC_LOADCTXSVAR:
                        loadVar(readUInt16(), ctx); break;
                    case BC_STORECTXDVAR: case BC_STORECTXIVAR: case BC_STORECTXSVAR:
                        storeVar(readUInt16(), ctx); break;
                    default: break;
                }
                break;
            }
            
            case BC_DCMP: BIN_D(-); break;
            case BC_ICMP: BIN_I(-); break;
                
            case BC_JA: _bci += readInt16() - 2; break;
            
#           define CMPJ(r) { \
                int top = popInt64(); \
                _bci += top r popInt64()? readInt16() - 2: 2; \
            }
            case BC_IFICMPNE: CMPJ(!=); break;
            case BC_IFICMPE: CMPJ(==); break;   
            case BC_IFICMPG: CMPJ(>); break;
            case BC_IFICMPGE: CMPJ(>=); break;
            case BC_IFICMPL: CMPJ(<); break;
            case BC_IFICMPLE: CMPJ(<=); break;
                
            case BC_DUMP: cout << top(); break;
            
            case BC_STOP: return_(); return new Status(); break;
                
            case BC_CALL: call(readUInt16()); break;     
          
            case BC_CALLNATIVE: callNative(readUInt16()); break;
            
            case BC_RETURN: return_(); break;
            
            case BC_BREAK: DEBUG(insnName(insn) << " is not yet implemented."); break;
                
            default: new Status(string("Unknown instruction ") + insnName(insn)); break;
        }
    }
    
    return new Status;
}

void InterpreterCodeImpl::loadVar(uint16_t varId) {
    loadVar(varId, _context);
}

void InterpreterCodeImpl::storeVar(uint16_t varId) {
    storeVar(varId, _context);
}

void InterpreterCodeImpl::loadVar(uint16_t varId, Context* context) {
    VERBOSE("  loadVar " << varId);
    push(*context->getVar(varId));
}

void InterpreterCodeImpl::storeVar(uint16_t varId, Context* context) {
    VERBOSE("  storeVar " << varId);
    *context->getVar(varId) = pop();
    INFO("  stored " << *context->getVar(varId));
}

InterpreterCodeImpl::Context* InterpreterCodeImpl::findContext(uint16_t ctxId) {
    Context* ctx = _context;
    while (ctx && ctx->id() != ctxId) {
        ctx = ctx->parent();
    }
    return ctx;
}

void InterpreterCodeImpl::call(uint16_t funcId) {
    DEBUG("Calling function " << funcId)
    if (_context)
        _context->saveBci(_bci);
    
    BytecodeFunction* func = (BytecodeFunction*) functionById(funcId);
    _context = new Context(_context, func);
    _bc = func->bytecode();
    _bci = 0;
}

void InterpreterCodeImpl::return_() {
    Context* oldCtx = _context;
    _context = oldCtx->parent();
    delete oldCtx;
    if (_context) {
        _bc = _context->bc();
        _bci = _context->bci();
    }
}

//template <unsigned N>
//struct applyVariadic {
//    template <typename Ret, typename... Args, typename... ArgsT>
//    static Ret call(Ret (*func)(Args...), void** v, ArgsT... args) {
//        return applyVariadic<N-1>::call(func, v, args..., 
//            *static_cast<typename std::tuple_element<sizeof...(args), 
//                std::tuple<Args...>>::type*>(v[sizeof...(ArgsT)]));
//    }
//};
//
//template<>
//struct applyVariadic<0> {
//    template <typename Ret, typename... Args, typename... ArgsT>
//    static Ret call(Ret (*func)(Args...), void** v, ArgsT... args) {
//        Ret r = func(args...);
////        INFO("sizeof args = " << sizeof...(args));
////        INFO("!!" << r);
//        return r;
//    }
//};
//
//template <typename Ret, typename... Args>
//static Ret callVariadic(Ret (*func)(Args...), void** v) {   
//    return applyVariadic<sizeof...(Args)>::call(func, v);
//}

union NativeArg {
    int64_t int_;
    char* string;
    double double_;
};

void InterpreterCodeImpl::callNative(uint16_t funcId) {
    int MAX_ARGS = 6;
    
    const Signature* signature;
    const string* name;
    const void* funAddr = nativeById(funcId, &signature, &name);
    INFO("  Calling native '" << *name << "' with args:");
    
    VarType retType = (*signature)[0].first; 
    uint16_t paramNum = signature->size() - 1;
    NativeArg args[MAX_ARGS];   
    
    for (uint16_t i = 0; i < paramNum && i < MAX_ARGS; ++i) {
        Val val = *_context->getVar(i);        
        VarType type = (*signature)[i + 1].first;
        switch (type) {
            case VT_INT: args[i].int_ = val.int64; break;
                
            case VT_STRING: args[i].string = constantById(val.uint16); break;
                
            case VT_DOUBLE: args[i].double_ = val.double_; break;
                
            default: assert(false); break;
        }
    };
    
    double doubleRet;
    int64_t intRet;
    asm ("mov %0, %%rdi;"::"r"(args[0].int_));
    asm ("mov %0, %%rsi;"::"r"(args[1].int_));
    asm ("mov %0, %%rdx;"::"r"(args[2].int_));
    asm ("mov %0, %%rcx;"::"r"(args[3].int_));
    asm ("mov %0, %%r8;"::"r"(args[4].int_));
    asm ("mov %0, %%r9;"::"r"(args[5].int_));
    asm ("movsd %0, %%xmm0;"::"m"(args[0].double_));
    asm ("movsd %0, %%xmm1;"::"m"(args[1].double_));
    asm ("movsd %0, %%xmm2;"::"m"(args[2].double_));
    asm ("movsd %0, %%xmm3;"::"m"(args[3].double_));
    asm ("movsd %0, %%xmm4;"::"m"(args[4].double_));
    asm ("movsd %0, %%xmm5;"::"m"(args[5].double_));
    asm ("call *%[fun];"
         "mov %%rax, %[iRet];"
         "movsd %%xmm0, %[dRet];"
         :[iRet]"=&r"(intRet),
          [dRet]"=m"(doubleRet)
         :[fun]"r"(funAddr)
          );
   
    switch (retType) {
        case VT_VOID: break;
        
        case VT_INT: pushInt64(intRet); break;
            
        case VT_STRING: pushUInt16(makePointer((char*) intRet)); break;
            
        case VT_DOUBLE: pushDouble(doubleRet); break;    
        
        default: assert(false); break;
    }      
}

uint16_t InterpreterCodeImpl::makePointer(char* ptr) {
    // just generate an id, underlying data is not our concern:
    uint16_t id = makeStringConstant("");  
    
    pointers.insert(std::make_pair(id, ptr));
    return id;
}

char* InterpreterCodeImpl::constantById(uint16_t id) {      
    if (pointers.size() > 0) { 
        map<uint16_t, char*>::iterator it = pointers.find(id);
        if (it != pointers.end()) {
            return it->second;
        }
    }  
    return const_cast<char*>(Code::constantById(id).c_str());
}


void printInsn(Bytecode* bc, size_t bci, ostream& out, int indent) {
    Instruction insn = bc->getInsn(bci);
    const char* name = insnName(insn); 
    out << string(indent, ' ') << bci << ": ";   
    switch (insn) {
      case BC_DLOAD:
          out << name << " " << bc->getDouble(bci + 1);
          break;
      case BC_ILOAD:
          out << name << " " << bc->getInt64(bci + 1);
          break;
      case BC_SLOAD:
          out << name << " @" << bc->getUInt16(bci + 1);
          break;
      case BC_CALL:
      case BC_CALLNATIVE:
          out << name << " *" << bc->getUInt16(bci + 1);
          break;
      case BC_LOADDVAR:
      case BC_STOREDVAR:
      case BC_LOADIVAR:
      case BC_STOREIVAR:
      case BC_LOADSVAR:
      case BC_STORESVAR:
          out << name << "!!!" << " @" << bc->getUInt16(bci + 1);
          break;
      case BC_LOADCTXDVAR:
      case BC_STORECTXDVAR:
      case BC_LOADCTXIVAR:
      case BC_STORECTXIVAR:
      case BC_LOADCTXSVAR:
      case BC_STORECTXSVAR:
          out << name << " @" << bc->getUInt16(bci + 1)
              << ":" << bc->getUInt16(bci + 3);
          break;
      case BC_IFICMPNE:
      case BC_IFICMPE:
      case BC_IFICMPG:
      case BC_IFICMPGE:
      case BC_IFICMPL:
      case BC_IFICMPLE:
      case BC_JA:
          out << name << " " << bc->getInt16(bci + 1) + bci + 1;
          break;
      default:
          out << name;
          break;
    }
    out << std::endl;
}

}







