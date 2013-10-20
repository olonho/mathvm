#include "bcinterpreter.h"

#include <dlfcn.h>

#include "AsmJit/Compiler.h"
#include "AsmJit/Logger.h"

using namespace mathvm;

#define CMP_OP(op) { \
    int32_t a = popStack().getInt(); \
    int32_t b = popStack().getInt(); \
    currentIP() += a op b ? getInt16() : 2; \
    }

#define SEQ_READ_AND_APPLY(func) { \
    uint16_t a = getUInt16(); \
    uint16_t b = getUInt16(); \
    func(a, b); \
    }

struct AsmJitVar {
    AsmJitVar(VarType t, AsmJit::GPVar *g = 0) : gp(g), type(t) {}
    AsmJitVar(VarType t, AsmJit::XMMVar *x = 0) : xmm(x), type(t) {}

    union {
        AsmJit::GPVar *gp;
        AsmJit::XMMVar *xmm;
    };

    VarType type;
};

void InterpreterCodeImpl::callNative(uint16_t id) {
    typedef void (*NFV)();
    typedef int64_t (*NFI)();
    typedef double (*NFD)();
    typedef char* (*NFS)();

    const Signature *sign;
    const std::string *name;
    const void *func = nativeById(id, &sign, &name);
    if(!func) throw std::string("Native function not found");

    using namespace AsmJit;
    Compiler c;

    switch(sign->at(0).first) {
    case VT_DOUBLE: c.newFunction(CALL_CONV_DEFAULT, FunctionBuilder0<double>()); break;
    case VT_VOID: c.newFunction(CALL_CONV_DEFAULT, FunctionBuilder0<void>()); break;
    case VT_STRING: c.newFunction(CALL_CONV_DEFAULT, FunctionBuilder0<char*>()); break;
    default: c.newFunction(CALL_CONV_DEFAULT, FunctionBuilder0<int64_t>());
    }

    c.getFunction()->setHint(FUNCTION_HINT_NAKED, true);

    FunctionBuilderX builder;
    std::vector<AsmJitVar> argv;
    uint32_t argc = sign->size() - 1;
    for(uint32_t i = argc, j = 0; i > 0; --i, ++j) {
        VarType t = sign->at(i).first;
        switch(t) {
        case VT_DOUBLE:
            argv.push_back(AsmJitVar(t, new XMMVar(c.newXMM(VARIABLE_TYPE_XMM_1D)))); //doubles should be in XMM vars
            builder.addArgument<double>();
//            double val = currentScope->vars[j].getDouble();
            c.movq(*argv[j].xmm, ptr_abs(currentScope->vars[j].getDoublePtr()));
            break;
        case VT_STRING:
            argv.push_back(AsmJitVar(t, new GPVar(c.newGP())));
            builder.addArgument<int64_t>();
//            char * val = currentScope->vars[j].getStringPtr();
//            c.mov(*argv[j].gp, AsmJit::imm((size_t)currentScope->vars[i - 1].getStringPtr()->data()));
            c.mov(*argv[j].gp, imm((size_t)currentScope->vars[j].getStringPtr()));
            break;
        default:
            argv.push_back(AsmJitVar(t, new GPVar(c.newGP())));
            builder.addArgument<int64_t>();
//            int64_t val = currentScope->vars[j].getInt();
            c.mov(*argv[j].gp, imm(currentScope->vars[j].getInt()));
            break;
        }
    }

    switch(sign->at(0).first) {
    case VT_DOUBLE: builder.setReturnValue<double>(); break;
    case VT_VOID: builder.setReturnValue<void>(); break;
    default: builder.setReturnValue<int64_t>(); break;
    }

    ECall *fcall = c.call(imm((size_t)func));
    fcall->setPrototype(CALL_CONV_DEFAULT, builder);

    for(uint32_t i = 0; i < argv.size(); ++i) {
        switch(argv[i].type) {
        case VT_DOUBLE: fcall->setArgument(i, *argv[i].xmm); break;
        default: fcall->setArgument(i, *argv[i].gp); break;
        }
    }

    AsmJitVar res(sign->at(0).first, (GPVar*)0);
    switch(res.type) {
    case VT_DOUBLE:
        res.xmm = new XMMVar(c.newXMM(VARIABLE_TYPE_XMM_1D));
        fcall->setReturn(*res.xmm);
        break;
    case VT_VOID:
        break;
    default:
        res.gp = new GPVar(c.newGP());
        fcall->setReturn(*res.gp);
        break;
    }

    for(size_t i = 0; i < argv.size(); ++i) {
        switch(argv[i].type) {
        case VT_DOUBLE: c.unuse(*argv[i].xmm); break;
        default: c.unuse(*argv[i].gp); break;
        }
    }

    switch(res.type) {
    case VT_DOUBLE: c.ret(*res.xmm); break;
    case VT_VOID: break;
    default: c.ret(*res.gp);
    }

    c.endFunction();

    switch(res.type) {
    case VT_DOUBLE: pstack.push(((NFD)c.make())()); break;
    case VT_STRING: pstack.push(((NFS)c.make())()); break;
    case VT_VOID: ((NFV)c.make())(); break;
    default: pstack.push(((NFI)c.make())());
    }
}

//----------------------------------------------------------------

Status *InterpreterCodeImpl::execute(vector<Var *> &vars) {
    currentScope = 0;
    callFunction(0);

    while(true) {
        switch(currentBC()->getInsn(currentIP()++)) {
        case BC_INVALID: return new Status("Invalid operation found");
        case BC_DLOAD: pstack.push(getDouble()); break;
        case BC_ILOAD: pstack.push(getInt64()); break;
        case BC_SLOAD: pstack.push(constantById(getUInt16()).data()); break;
        case BC_DLOAD0: pstack.push(0.0); break;
        case BC_ILOAD0: pstack.push((int64_t)0); break;
        case BC_SLOAD0: pstack.push((int64_t)0); break;
        case BC_DLOAD1: pstack.push(1.0); break;
        case BC_ILOAD1: pstack.push((int64_t)1); break;
        case BC_DLOADM1: pstack.push(-1.0); break;
        case BC_ILOADM1: pstack.push((int64_t)(-1)); break;

        case BC_DADD: doubleBinOp(tADD); break;
        case BC_IADD: intBinOp(tADD); break;
        case BC_DSUB: doubleBinOp(tSUB); break;
        case BC_ISUB: intBinOp(tSUB); break;
        case BC_DMUL: doubleBinOp(tMUL); break;
        case BC_IMUL: intBinOp(tMUL); break;
        case BC_DDIV: doubleBinOp(tDIV); break;
        case BC_IDIV: intBinOp(tDIV); break;
        case BC_IMOD: intBinOp(tMOD); break;
        case BC_IAAND: intBinOp(tAAND); break;
        case BC_IAOR: intBinOp(tAOR); break;
        case BC_IAXOR: intBinOp(tAXOR); break;
        case BC_DNEG: pstack.push(-popStack().getDouble()); break;
        case BC_INEG: pstack.push(-popStack().getInt()); break;

        case BC_I2D: pstack.push((double)popStack().getInt()); break;
        case BC_D2I: pstack.push((int64_t)popStack().getDouble()); break;
        case BC_S2I: pstack.push((int64_t)popStack().getStringPtr()); break;

        case BC_SWAP: {
            Any a = popStack();
            Any b = popStack();
            pstack.push(a);
            pstack.push(b);
            break;
        }
        case BC_POP: pstack.pop(); break;

        case BC_IPRINT: std::cout << popStack().getInt(); break;
        case BC_DPRINT: std::cout << popStack().getDouble(); break;
        case BC_SPRINT: std::cout << popStack().getString(); break;

        case BC_LOADIVAR0: case BC_LOADDVAR0: case BC_LOADSVAR0: loadVar(0); break;
        case BC_LOADIVAR1: case BC_LOADDVAR1: case BC_LOADSVAR1: loadVar(1); break;
        case BC_LOADIVAR2: case BC_LOADDVAR2: case BC_LOADSVAR2: loadVar(2); break;
        case BC_LOADIVAR3: case BC_LOADDVAR3: case BC_LOADSVAR3: loadVar(3); break;
        case BC_LOADIVAR: case BC_LOADDVAR: case BC_LOADSVAR: loadVar(getUInt16()); break;
        case BC_LOADCTXIVAR: case BC_LOADCTXDVAR: case BC_LOADCTXSVAR: SEQ_READ_AND_APPLY(loadVar); break;

        case BC_STOREIVAR0: case BC_STOREDVAR0: case BC_STORESVAR0: storeVar(0); break;
        case BC_STOREIVAR1: case BC_STOREDVAR1: case BC_STORESVAR1: storeVar(1); break;
        case BC_STOREIVAR2: case BC_STOREDVAR2: case BC_STORESVAR2: storeVar(2); break;
        case BC_STOREIVAR3: case BC_STOREDVAR3: case BC_STORESVAR3: storeVar(3); break;
        case BC_STOREIVAR: case BC_STOREDVAR: case BC_STORESVAR: storeVar(getUInt16()); break;
        case BC_STORECTXIVAR: case BC_STORECTXDVAR: case BC_STORECTXSVAR: SEQ_READ_AND_APPLY(storeVar); break;

        case BC_DCMP: doubleBinOp(tEQ); break;
        case BC_ICMP: intBinOp(tEQ); break;

        case BC_JA: currentIP() += getInt16(); break;
        case BC_IFICMPE: CMP_OP(==); break;
        case BC_IFICMPG: CMP_OP(>); break;
        case BC_IFICMPL: CMP_OP(<); break;
        case BC_IFICMPNE: CMP_OP(!=); break;
        case BC_IFICMPGE: CMP_OP(>=); break;
        case BC_IFICMPLE: CMP_OP(<=); break;

//        case BC_DUMP: std::cout << pstack.top().getDouble() << std::endl; break;
        case BC_DUMP: throw std::string("Unsupported operation: BC_DUMP");  //tos type unknown
        case BC_STOP: return 0;
        case BC_CALL: callFunction(getUInt16()); break;
        case BC_CALLNATIVE: callNative(getUInt16()); break;

        case BC_RETURN: {
            IScope *parentScope = currentScope->parent;
            delete currentScope;
            currentScope = parentScope;
            break;
        }

        case BC_BREAK: std::cout << "BC_BREAK is not implemented" << std::endl; break;

        default: return new Status("Unsupported operation");
        }
    }
    return 0;
}

void InterpreterCodeImpl::doubleBinOp(TokenKind op) {
    double a = popStack().getDouble();
    double b = popStack().getDouble();

    switch(op) {
    case tADD: pstack.push(a + b); break;
    case tSUB: pstack.push(a - b); break;
    case tMUL: pstack.push(a * b); break;
    case tDIV: pstack.push(a / b); break;
    case tEQ: pstack.push((int64_t)(a == b)); break;
    default: throw std::string("Unsupported double operation: ") + tokenOp(op);
    }
}

void InterpreterCodeImpl::intBinOp(TokenKind op) {
    int64_t a = popStack().getInt();
    int64_t b = popStack().getInt();

    switch(op) {
    case tADD: pstack.push(a + b); break;
    case tSUB: pstack.push(a - b); break;
    case tMUL: pstack.push(a * b); break;
    case tDIV: pstack.push(a / b); break;
    case tMOD: pstack.push(a % b); break;
    case tAAND: pstack.push(a & b); break;
    case tAXOR: pstack.push(a ^ b); break;
    case tAOR: pstack.push(a | b); break;
    case tEQ: pstack.push((int64_t)(a == b)); break;
    default: throw std::string("Unsupported int operation: ") + tokenOp(op);
    }
}

inline void InterpreterCodeImpl::loadVar(uint16_t index) {
    pstack.push(currentScope->vars[index]);
}

inline void InterpreterCodeImpl::loadVar(uint16_t cid, uint16_t index) {
    IScope *s = currentScope->parent;
    while(s && s->function->id() != cid) s = s->parent;
    if(!s) throw std::string("BC_LOADCTX#VAR: context not found");
    pstack.push(s->vars[index]);
}

inline void InterpreterCodeImpl::storeVar(uint32_t index) {
    currentScope->vars[index] = popStack();
}

inline void InterpreterCodeImpl::storeVar(uint16_t cid, uint16_t index) {
    IScope *s = currentScope->parent;
    while(s && s->function->id() != cid) s = s->parent;
    if(!s) throw std::string("BC_STORECTX#VAR: context not found");
    s->vars[index] = popStack();
}
