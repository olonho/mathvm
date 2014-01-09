#include "CodeImpl.h"
#include "errors.h"

#include <iostream>

// macro

#define PUSH_UNION(val) \
    {_computationStack.push(val);}

#define PUSH(field, val) \
    { \
        Val v; \
        v.field = val; \
        _computationStack.push(v); \
    } 

#define PUSH_INT(val) \
    PUSH(ival, val)

#define PUSH_DOUBLE(val) \
    PUSH(dval, val)

#define PUSH_STRING(val) \
    PUSH(sval, val)

#define CHECK_STACK \
    if (_computationStack.empty()) {throw stackUnderFlowError(_fid, _ip);}

#define POP_UNION(var) \
    CHECK_STACK; \
    Val var = _computationStack.top(); \
    _computationStack.pop(); 

#define POP(var, field) \
    CHECK_STACK; \
    var = _computationStack.top().field; \
    _computationStack.pop();

#define BINARY_OP(pusher, type, field, op) \
    { \
        type upper, lower; \
        POP(upper, field); \
        POP(lower, field); \
        pusher(upper op lower); \
    } 

#define UNARY_OP(pusher, type, field, op) \
    { \
        type top; \
        POP(top, field); \
        pusher(op top); \
    }

#define CMPX(comparator) \
    { \
        DECL_AND_POP(int64_t, ival); \
        int16_t offset = _bc->getInt16(_ip + 1); \
        if (comparator) { \
            _ip += offset + 1; \
            continue; \
        } \
    }


// utils

template <typename T>
int64_t compare(T arg1, T arg2) {
    if (arg1 == arg2) {
        return 0;
    }
    return arg1 > arg2 ? 1 : -1;
}

using namespace mathvm;

// init

void CodeImpl::initialize() {
    _fid = 0;
    _ip = 0;
    _iReg.assign(4, Val());
    _dReg.assign(4, Val());
    _sReg.assign(4, Val());

    Code::FunctionIterator it(this);
    while (it.hasNext()) {
        TranslatedFunction* f = it.next();
        ContextStack s;
        s.push(VariableStack(f->localsNumber() + f->parametersNumber()));
        _functions.push_back(s);
    }

    _bc = ((BytecodeFunction*)functionById(0))->bytecode();

}

// checkers

void CodeImpl::validate() {
    if (_functions.empty()) {
        throw error("There is no top function. ");
    }
}

// run

Status* CodeImpl::execute() {
    try {
        executeMachine();

        return 0;
    } catch (error& e) {
        return new Status(e.what());
    }
}

void CodeImpl::executeMachine() {
    initialize();

    bool next = true;

    while (next && _ip < _bc->length()) {
        Instruction insn = _bc->getInsn(_ip);
        size_t length;
        bytecodeName(insn, &length);
        switch (insn) {
            case BC_INVALID: throw invalidBytecodeError(_fid, _ip); break;
            case BC_DLOAD: PUSH_DOUBLE(_bc->getDouble(_ip + 1)); break;
            case BC_ILOAD: PUSH_INT(_bc->getInt64(_ip + 1)); break;
            case BC_SLOAD: PUSH_STRING(_bc->getUInt16(_ip + 1)); break;
            case BC_DLOAD0: PUSH_DOUBLE(0); break;
            case BC_ILOAD0: PUSH_INT(0); break;
            case BC_SLOAD0: PUSH_STRING(0); break;
            case BC_DLOAD1: PUSH_DOUBLE(1); break;
            case BC_ILOAD1: PUSH_INT(1); break;
            case BC_DLOADM1: PUSH_DOUBLE(-1); break;
            case BC_ILOADM1: PUSH_INT(-1); break;
            case BC_DADD: BINARY_OP(PUSH_DOUBLE, double, dval, +); break;
            case BC_IADD: BINARY_OP(PUSH_INT, int64_t, ival, +); break;
            case BC_DSUB: BINARY_OP(PUSH_DOUBLE, double, dval, -); break;
            case BC_ISUB: BINARY_OP(PUSH_INT, int64_t, ival, -); break;
            case BC_DMUL: BINARY_OP(PUSH_DOUBLE, double, dval, *); break;
            case BC_IMUL: BINARY_OP(PUSH_INT, int64_t, ival, *); break;
            case BC_DDIV: BINARY_OP(PUSH_DOUBLE, double, dval, /); break;
            case BC_IDIV: BINARY_OP(PUSH_INT, int64_t, ival, /); break;
            case BC_IMOD: BINARY_OP(PUSH_INT, int64_t, ival, %); break;
            case BC_DNEG: UNARY_OP(PUSH_DOUBLE, double, dval, -); break;
            case BC_INEG: UNARY_OP(PUSH_INT, int64_t, ival, -); break;
            case BC_IAOR: BINARY_OP(PUSH_INT, int64_t, ival, |); break;
            case BC_IAAND: BINARY_OP(PUSH_INT, int64_t, ival, &); break;
            case BC_IAXOR: BINARY_OP(PUSH_INT, int64_t, ival, ^); break;
            case BC_IPRINT: {
                int64_t val;
                POP(val, ival);
                cout << val;
                break;
            }
            case BC_DPRINT: {
                double val;
                POP(val, dval);
                cout << val;
                break;
            }
            case BC_SPRINT: {
                uint16_t id;
                POP(id, sval);
                cout << constantById(id);
                break;
            }
            case BC_I2D: {
                int64_t top;
                POP(top, ival);
                PUSH_DOUBLE(top);
                break;
            }
            case BC_D2I: {
                double top;
                POP(top, dval);
                PUSH_INT(top);
                break;
            }
            case BC_S2I: {
                uint16_t id;
                POP(id, sval);
                string val = constantById(id);
                stringstream stream;
                stream << val;
                int64_t res;
                stream >> res;
                PUSH_INT(res);
                break;
            }
            case BC_SWAP: {
                POP_UNION(upper);
                POP_UNION(lower);
                PUSH_UNION(lower);
                PUSH_UNION(upper);
                break;
            }
            case BC_POP: {
                CHECK_STACK;
                _computationStack.pop();
                break;
            }

            // register operations 

            #define FOR_VARS(DO, insn, reg) \
                DO(insn, 0, reg) \
                DO(insn, 1, reg) \
                DO(insn, 2, reg) \
                DO(insn, 3, reg)

            // load register operations

            #define CASE_ELEM(insn, index, reg) \
                case BC_##insn##VAR##index: { \
                    PUSH_UNION(reg[index]); \
                    break; \
                }

            FOR_VARS(CASE_ELEM, LOADD, _dReg)
            FOR_VARS(CASE_ELEM, LOADI, _iReg)
            FOR_VARS(CASE_ELEM, LOADS, _sReg)

            #undef CASE_ELEM

            // store register operations

            #define CASE_ELEM(insn, index, reg) \
                case BC_##insn##VAR##index: { \
                    POP_UNION(top); \
                    reg[index] = top; \
                    break; \
                }

            FOR_VARS(CASE_ELEM, STORED, _dReg) 
            FOR_VARS(CASE_ELEM, STOREI, _iReg)
            FOR_VARS(CASE_ELEM, STORES, _sReg)

            #undef CASE_ELEM
            #undef FOR_VARS

            // load variable operations

            #define CASE_ELEM(insn) \
                case BC_##insn##VAR: { \
                    uint16_t id = _bc->getUInt16(_ip + 1); \
                    Val val = _functions[_fid].top()[id]; \
                    PUSH_UNION(val); \
                    break; \
                }

            #define FOR_VARS(DO, prefix) \
                DO(prefix##D) \
                DO(prefix##I) \
                DO(prefix##S)

            FOR_VARS(CASE_ELEM, LOAD)

            #undef CASE_ELEM

            // store variable operations

            #define CASE_ELEM(insn) \
                case BC_##insn##VAR: { \
                    uint16_t id = _bc->getUInt16(_ip + 1); \
                    POP_UNION(var); \
                    _functions[_fid].top()[id] = var; \
                    break; \
                } 

            FOR_VARS(CASE_ELEM, STORE)

            #undef CASE_ELEM

            // load variable by context operations

            #define CASE_ELEM(insn) \
                case BC_##insn##VAR: { \
                    uint16_t id = _bc->getUInt16(_ip + 1); \
                    uint16_t ctx = _bc->getUInt16(_ip + 3); \
                    Val val = _functions[ctx].top()[id]; \
                    PUSH_UNION(val); \
                    break; \
                }

            FOR_VARS(CASE_ELEM, LOADCTX)

            #undef CASE_ELEM

            // store variable by context operations

            #define CASE_ELEM(insn) \
                case BC_##insn##VAR: { \
                    uint16_t id = _bc->getUInt16(_ip + 1); \
                    uint16_t ctx = _bc->getUInt16(_ip + 3); \
                    POP_UNION(var); \
                    _functions[ctx].top()[id] = var; \
                    break; \
                }

            FOR_VARS(CASE_ELEM, STORECTX)

            #undef CASE_ELEM
            #undef FOR_VARS

            #define DECL_AND_POP(type, field) \
                type upper; \
                type lower; \
                POP(upper, field); \
                POP(lower, field); \

            case BC_DCMP: {
                DECL_AND_POP(double, dval);
                PUSH_INT(compare(upper, lower));
                break;
            }
            case BC_ICMP: {
                DECL_AND_POP(int64_t, ival);
                PUSH_INT(compare(upper, lower));
                break;
            }
            case BC_JA: {
                int16_t offset = _bc->getInt16(_ip + 1);
                _ip += offset + 1;
                continue;
            }
            case BC_IFICMPNE: CMPX(upper != lower); break; 
            case BC_IFICMPE: CMPX(upper == lower); break;
            case BC_IFICMPG: CMPX(upper > lower); break;
            case BC_IFICMPGE: CMPX(upper >= lower); break;
            case BC_IFICMPL: CMPX(upper < lower); break;
            case BC_IFICMPLE: CMPX(upper <= lower); break;
         
            #undef DECL_AND_POP

            case BC_STOP: throw notImplementedError(_fid, _ip); break;
            case BC_DUMP: throw notImplementedError(_fid, _ip); break;

            case BC_CALL: {
                uint16_t callId = _bc->getUInt16(_ip + 1);
                _functions[callId].push(VariableStack(_functions[callId].top()));
                _functionStackTrace.push(make_pair(_fid, _ip + 3));

                _ip = 0;
                _fid = callId;
                _bc = ((BytecodeFunction*)functionById(callId))->bytecode();
                continue;
            }
         
            case BC_CALLNATIVE: throw notImplementedError(_fid, _ip); break;
         
            case BC_RETURN: {
                if (_fid == 0) {
                    next = false;
                    continue;
                }
                if (_functionStackTrace.empty()) {
                    throw stackUnderFlowError(_fid, _ip);
                }

                pair<uint16_t, uint32_t> top = _functionStackTrace.top();
                uint16_t retId = top.first;
                uint32_t retIp = top.second;

                _functionStackTrace.pop();
                _functions[_fid].pop();

                _bc = ((BytecodeFunction*)functionById(retId))->bytecode();

                _fid = retId; 
                _ip = retIp; 
                continue;
            }
       
            case BC_BREAK: throw notImplementedError(_fid, _ip); break;
     
            default: throw invalidBytecodeError(_fid, _ip);
        }
        _ip += length;
    }
     
}

Status* CodeImpl::execute(vector<Var*> & vars) {
    /* NOT IMPLEMENTED */
    assert(false);
    return 0;
}

#undef PUSH_UNION
#undef PUSH
#undef PUSH_INT
#undef PUSH_DOUBLE
#undef PUSH_STRING
#undef CHECK_STACK
#undef POP_UNION
#undef POP
#undef BINARY_OP
#undef CMPX