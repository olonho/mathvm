#include "CodeImpl.h"
#include "errors.h"

#include <iostream>


#define PUSH_UNION(val) \
    {_stack.push(val);}

#define PUSH(field, val) \
    {Val v; v.field = val; _stack.push(v);}

#define PUSH_INT(val) \
    PUSH(ival, val)

#define PUSH_DOUBLE(val) \
    PUSH(dval, val)

#define PUSH_STRING(val) \
    PUSH(sval, val)

#define CHECK_STACK \
    if (_stack.empty()) {throw stackUnderFlowError(_ip);}

#define POP_UNION(var) \
    CHECK_STACK; Val var = _stack.top(); _stack.pop();

#define POP(var, field) \
    CHECK_STACK; var = _stack.top().field; _stack.pop();

#define BINARY_OP(pusher, type, field, op) \
    {type upper, lower; POP(upper, field); POP(lower, field); pusher(upper op lower);} 

#define UNARY_OP(pusher, type, field, op) \
    {type top; POP(top, field); pusher(op top);}


using namespace mathvm;

void CodeImpl::validate() {
    if (_cStack.empty()) {
        throw error("There is no top function. ");
    }
}

void CodeImpl::initialize() {
    _fid = 0;
    _ip = 0;

    Code::FunctionIterator it(this);
    while (it.hasNext()) {
        TranslatedFunction* f = it.next();
        FStack s;
        s.push(VStack(f->localsNumber() + f->parametersNumber()));
        _cStack.push_back(s);
    }


}

Status* CodeImpl::execute() {
    initialize();
    bool next = true;
    uint32_t saveIp = _ip;
    uint16_t saveFid = _fid;

    while (next) {
        Instruction insn = _bc->getInsn(_ip);
        size_t length;
        bytecodeName(insn, &length);
        switch (insn) {
            case BC_INVALID: {
                throw invalidBytecodeError(functionById(_fid)->name(), _ip);
                break;
            }
         
            case BC_DLOAD: {
                PUSH_DOUBLE(_bc->getDouble(_ip + 1));
                break;
            }
         
            case BC_ILOAD: {
                PUSH_INT(_bc->getInt64(_ip + 1));
                break;
            }
         
            case BC_SLOAD: {
                PUSH_STRING(_bc->getUInt16(_ip + 1));
                break;
            }
         
            case BC_DLOAD0: {
                PUSH_DOUBLE(0);
                break;
            }
         
            case BC_ILOAD0: {
                PUSH_INT(0);
                break;
            }
         
            case BC_SLOAD0: {
                PUSH_STRING(0);
                break;
            }
         
            case BC_DLOAD1: {
                PUSH_DOUBLE(1);
                break;
            }
         
            case BC_ILOAD1: {
                PUSH_INT(1);
                break;
            }
         
            case BC_DLOADM1: {
                PUSH_DOUBLE(-1);
                break;
            }
         
            case BC_ILOADM1: {
                PUSH_INT(-1);
                break;
            }
         
            case BC_DADD: {
                BINARY_OP(PUSH_DOUBLE, double, dval, +);
                break;
            }
         
            case BC_IADD: {
                BINARY_OP(PUSH_INT, int64_t, ival, +);
                break;
            }
         
            case BC_DSUB: {
                BINARY_OP(PUSH_DOUBLE, double, dval, -);
                break;
            }
         
            case BC_ISUB: {
                BINARY_OP(PUSH_INT, int64_t, ival, -);
                break;
            }
         
            case BC_DMUL: {
                BINARY_OP(PUSH_DOUBLE, double, dval, *);
                break;
            }
         
            case BC_IMUL: {
                BINARY_OP(PUSH_INT, int64_t, ival, *);
                break;
            }
         
            case BC_DDIV: {
                BINARY_OP(PUSH_DOUBLE, double, dval, /);
                break;
            }
         
            case BC_IDIV: {
                BINARY_OP(PUSH_INT, int64_t, ival, /);
                break;
            }
         
            case BC_IMOD: {
                BINARY_OP(PUSH_INT, int64_t, ival, %);
                break;
            }
         
            case BC_DNEG: {
                UNARY_OP(PUSH_DOUBLE, double, dval, -);
                break;
            }
         
            case BC_INEG: {
                UNARY_OP(PUSH_INT, int64_t, ival, -);
                break;
            }
         
            case BC_IAOR: {
                BINARY_OP(PUSH_INT, int64_t, ival, |);
                break;
            }
         
            case BC_IAAND: {
                BINARY_OP(PUSH_INT, int64_t, ival, &);
                break;
            }
         
            case BC_IAXOR: {
                BINARY_OP(PUSH_INT, int64_t, ival, ^);
                break;
            }
         
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
                _stack.pop();
                break;
            }
         
            case BC_LOADDVAR0: {

                break;
            }
         
            case BC_LOADDVAR1: {

                break;
            }
         
            case BC_LOADDVAR2: {

                break;
            }
         
            case BC_LOADDVAR3: {

                break;
            }
         
            case BC_LOADIVAR0: {

                break;
            }
         
            case BC_LOADIVAR1: {

                break;
            }
         
            case BC_LOADIVAR2: {

                break;
            }
         
            case BC_LOADIVAR3: {

                break;
            }
         
            case BC_LOADSVAR0: {

                break;
            }
         
            case BC_LOADSVAR1: {

                break;
            }
         
            case BC_LOADSVAR2: {

                break;
            }
         
            case BC_LOADSVAR3: {

                break;
            }
         
            case BC_STOREDVAR0: {

                break;
            }
         
            case BC_STOREDVAR1: {

                break;
            }
         
            case BC_STOREDVAR2: {

                break;
            }
         
            case BC_STOREDVAR3: {

                break;
            }
         
            case BC_STOREIVAR0: {

                break;
            }
         
            case BC_STOREIVAR1: {

                break;
            }
         
            case BC_STOREIVAR2: {

                break;
            }
         
            case BC_STOREIVAR3: {

                break;
            }
         
            case BC_STORESVAR0: {

                break;
            }
         
            case BC_STORESVAR1: {

                break;
            }
         
            case BC_STORESVAR2: {

                break;
            }
         
            case BC_STORESVAR3: {

                break;
            }
         
            case BC_LOADDVAR: {

                break;
            }
         
            case BC_LOADIVAR: {

                break;
            }
         
            case BC_LOADSVAR: {

                break;
            }
         
            case BC_STOREDVAR: {

                break;
            }
         
            case BC_STOREIVAR: {

                break;
            }
         
            case BC_STORESVAR: {

                break;
            }
         
            case BC_LOADCTXDVAR: {

                break;
            }
         
            case BC_LOADCTXIVAR: {

                break;
            }
         
            case BC_LOADCTXSVAR: {

                break;
            }
         
            case BC_STORECTXDVAR: {

                break;
            }
         
            case BC_STORECTXIVAR: {

                break;
            }
         
            case BC_STORECTXSVAR: {

                break;
            }
         
            case BC_DCMP: {

                break;
            }
         
            case BC_ICMP: {

                break;
            }
         
            case BC_JA: {

                break;
            }
         
            case BC_IFICMPNE: {

                break;
            }
         
            case BC_IFICMPE: {

                break;
            }
         
            case BC_IFICMPG: {

                break;
            }
         
            case BC_IFICMPGE: {

                break;
            }
         
            case BC_IFICMPL: {

                break;
            }
         
            case BC_IFICMPLE: {

                break;
            }
         
            case BC_STOP: {

                break;
            }
         
            case BC_DUMP: {

                break;
            }
         
            case BC_CALL: {

                break;
            }
         
            case BC_CALLNATIVE: {

                break;
            }
         
            case BC_RETURN: {
                if (_fid == 0) {
                    next = false;
                }
                _ip = saveIp;
                _fid = saveFid;
                continue;
            }
       
            case BC_BREAK: {

                break;
            }
     
            default: break;
        }
        _ip += length;
    }
     
    return 0;
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