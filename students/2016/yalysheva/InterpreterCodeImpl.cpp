//
// Created by natalia on 19.11.16.
//

#include <stack>
#include "InterpreterCodeImpl.h"

namespace mathvm {

class InterpreterException : public std::exception {
    string const _message;
public:
    InterpreterException(string const &message) :
            _message("interpretation error: " + message) {}

    char const *what() const throw() {
        return _message.c_str();
    }

    ~InterpreterException() throw() {}
};


struct Interpreter {

    Interpreter(InterpreterCodeImpl *code) : _code(code), _vars(code) {
        _vars.push(0);
        _functions.push_back((BytecodeFunction *) _code->functionByName(AstFunction::top_name));
        _bcids.push_back(0);
    }

    void execute() {
        while (_bcids.back() < bytecode()->length()) {
            bool jmp = false;
            Value lhs;
            Value rhs;

            switch (bytecode()->getInsn(_bcids.back())) {

                case BC_DLOAD:
                    _stack.push(bytecode()->getDouble(_bcids.back() + 1));
                    break;
                case BC_ILOAD:
                    _stack.push(bytecode()->getInt64(_bcids.back() + 1));
                    break;
                case BC_SLOAD:
                    _stack.push(_code->constantById(bytecode()->getUInt16(_bcids.back() + 1)).c_str());
                    break;
                case BC_DLOAD0:
                    _stack.push(0.0);
                    break;
                case BC_ILOAD0:
                case BC_SLOAD0:
                    _stack.push((int64_t) 0);
                    break;
                case BC_DLOAD1:
                    _stack.push(1.0);
                    break;
                case BC_ILOAD1:
                    _stack.push((int64_t) 1);
                    break;
                case BC_DLOADM1:
                    _stack.push(-1.0);
                    break;
                case BC_ILOADM1:
                    _stack.push((int64_t) -1);
                    break;

#define BINARY_OP(type, op) \
lhs = popValue(); \
rhs = popValue(); \
_stack.push(lhs.type() op rhs.type()); \
break;
                case BC_DADD:
                BINARY_OP(getDouble, +)
                case BC_IADD:
                BINARY_OP(getInt, +)
                case BC_DSUB:
                BINARY_OP(getDouble, -)
                case BC_ISUB:
                BINARY_OP(getInt, -)
                case BC_DMUL:
                BINARY_OP(getDouble, *)
                case BC_IMUL:
                BINARY_OP(getInt, *)
                case BC_DDIV:
                BINARY_OP(getDouble, /)
                case BC_IDIV:
                BINARY_OP(getInt, /)
                case BC_IMOD:
                BINARY_OP(getInt, %)
                case BC_IAOR:
                BINARY_OP(getInt, |)
                case BC_IAAND:
                BINARY_OP(getInt, &)
                case BC_IAXOR:
                BINARY_OP(getInt, ^)
#undef BINARY_OP

                case BC_DNEG:
                    _stack.push(-popValue().getDouble());
                    break;
                case BC_INEG:
                    _stack.push(-popValue().getInt());
                    break;
                case BC_IPRINT:
                    write(std::cout, popValue(), VT_INT);
                    break;
                case BC_DPRINT:
                    write(std::cout, popValue(), VT_DOUBLE);
                    break;
                case BC_SPRINT:
                    write(std::cout, popValue(), VT_STRING);
                    break;
                case BC_I2D:
                    _stack.push((double) popValue().getInt());
                    break;
                case BC_D2I:
                    _stack.push((int64_t) popValue().getDouble());
                    break;
                case BC_S2I:
                    _stack.push((int64_t) (popValue().getString() != 0));
                    break;
                case BC_SWAP:
                    lhs = popValue();
                    rhs = popValue();
                    _stack.push(lhs);
                    _stack.push(rhs);
                    break;
                case BC_POP:
                    popValue();
                    break;
                case BC_LOADDVAR0:
                    _stack.push(_vars.load(1, 0).getDouble());
                    break;
                case BC_LOADDVAR1:
                    _stack.push(_vars.load(1, 1).getDouble());
                    break;
                case BC_LOADDVAR2:
                    _stack.push(_vars.load(1, 2).getDouble());
                    break;
                case BC_LOADDVAR3:
                    _stack.push(_vars.load(1, 3).getDouble());
                    break;
                case BC_LOADIVAR0:
                    _stack.push(_vars.load(1, 0).getInt());
                    break;
                case BC_LOADIVAR1:
                    _stack.push(_vars.load(1, 1).getInt());
                    break;
                case BC_LOADIVAR2:
                    _stack.push(_vars.load(1, 2).getInt());
                    break;
                case BC_LOADIVAR3:
                    _stack.push(_vars.load(1, 3).getInt());
                    break;
                case BC_LOADSVAR0:
                    _stack.push(_vars.load(1, 0).getString());
                    break;
                case BC_LOADSVAR1:
                    _stack.push(_vars.load(1, 1).getString());
                    break;
                case BC_LOADSVAR2:
                    _stack.push(_vars.load(1, 2).getString());
                    break;
                case BC_LOADSVAR3:
                    _stack.push(_vars.load(1, 3).getString());
                    break;
                case BC_STOREDVAR0:
                    _stack.push(_vars.load(1, 0).getDouble());
                    break;
                case BC_STOREDVAR1:
                    _stack.push(_vars.load(1, 1).getDouble());
                    break;
                case BC_STOREDVAR2:
                    _stack.push(_vars.load(1, 2).getDouble());
                    break;
                case BC_STOREDVAR3:
                    _stack.push(_vars.load(1, 3).getDouble());
                    break;
                case BC_STOREIVAR0:
                    _stack.push(_vars.load(1, 0).getInt());
                    break;
                case BC_STOREIVAR1:
                    _stack.push(_vars.load(1, 1).getInt());
                    break;
                case BC_STOREIVAR2:
                    _stack.push(_vars.load(1, 2).getInt());
                    break;
                case BC_STOREIVAR3:
                    _stack.push(_vars.load(1, 3).getInt());
                    break;
                case BC_STORESVAR0:
                    _stack.push(_vars.load(1, 0).getString());
                    break;
                case BC_STORESVAR1:
                    _stack.push(_vars.load(1, 1).getString());
                    break;
                case BC_STORESVAR2:
                    _stack.push(_vars.load(1, 2).getString());
                    break;
                case BC_STORESVAR3:
                    _stack.push(_vars.load(1, 3).getString());
                    break;
                case BC_LOADDVAR:
                    _stack.push(_vars.load(1, bytecode()->getUInt16(_bcids.back() + 1)).getDouble());
                    break;
                case BC_LOADIVAR:
                    _stack.push(_vars.load(1, bytecode()->getUInt16(_bcids.back() + 1)).getInt());
                    break;
                case BC_LOADSVAR:
                    _stack.push(_vars.load(1, bytecode()->getUInt16(_bcids.back() + 1)).getString());
                    break;
                case BC_STOREDVAR:
                    _vars.store(popValue().getDouble(), 1, bytecode()->getUInt16(_bcids.back() + 1));
                    break;
                case BC_STOREIVAR:
                    _vars.store(popValue().getInt(), 1, bytecode()->getUInt16(_bcids.back() + 1));
                    break;
                case BC_STORESVAR:
                    _vars.store(popValue().getString(), 1, bytecode()->getUInt16(_bcids.back() + 1));
                    break;

                case BC_LOADCTXDVAR:
                    _stack.push(_vars.load(bytecode()->getUInt16(_bcids.back() + 1),
                                                 bytecode()->getUInt16(_bcids.back() + 3)).getDouble());
                    break;
                case BC_LOADCTXIVAR:
                    _stack.push(_vars.load(bytecode()->getUInt16(_bcids.back() + 1),
                                                 bytecode()->getUInt16(_bcids.back() + 3)).getInt());
                    break;
                case BC_LOADCTXSVAR:
                    _stack.push(_vars.load(bytecode()->getUInt16(_bcids.back() + 1),
                                                 bytecode()->getUInt16(_bcids.back() + 3)).getString());
                    break;

                case BC_STORECTXDVAR:
                    _vars.store(popValue().getDouble(), bytecode()->getUInt16(_bcids.back() + 1),
                                bytecode()->getUInt16(_bcids.back() + 3));
                    break;
                case BC_STORECTXIVAR:
                    _vars.store(popValue().getInt(), bytecode()->getUInt16(_bcids.back() + 1),
                                bytecode()->getUInt16(_bcids.back() + 3));
                    break;
                case BC_STORECTXSVAR:
                    _vars.store(popValue().getString(), bytecode()->getUInt16(_bcids.back() + 1),
                                bytecode()->getUInt16(_bcids.back() + 3));
                    break;

#define CMP(type) \
lhs = popValue(); \
rhs = popValue(); \
if (lhs.type() < rhs.type()) \
    _stack.push((int64_t) -1); \
else if (lhs.type() == rhs.type()) \
    _stack.push((int64_t) 0); \
else \
    _stack.push((int64_t) 1); \
break;
                case BC_DCMP:
                CMP(getDouble)
                case BC_ICMP:
                CMP(getInt)
#undef CMP

                case BC_JA:
                    _bcids.back() += bytecode()->getInt16(_bcids.back() + 1) + 1;
                    jmp = true;
                    break;

#define CMP_JMP(op) \
lhs = popValue(); \
rhs = popValue(); \
if (lhs.getInt() op rhs.getInt()) { \
    _bcids.back() += bytecode()->getInt16(_bcids.back() + 1) + 1; \
    jmp = true; \
} \
break;
                case BC_IFICMPNE:
                CMP_JMP(!=)
                case BC_IFICMPE:
                CMP_JMP(==)
                case BC_IFICMPG:
                CMP_JMP(>)
                case BC_IFICMPGE:
                CMP_JMP(>=)
                case BC_IFICMPL:
                CMP_JMP(<)
                case BC_IFICMPLE:
                CMP_JMP(<=)
#undef CMP_JMP

                case BC_DUMP:
                    lhs = popValue();
                    write(std::cerr, lhs, lhs.type());
                    break;
                case BC_STOP:
                    return;
                case BC_CALL:
                    _vars.push(bytecode()->getUInt16(_bcids.back() + 1));
                    _functions.push_back(
                            (BytecodeFunction *) _code->functionById(bytecode()->getUInt16(_bcids.back() + 1)));
                    _bcids.push_back(0);
                    jmp = true;
                    break;
                case BC_CALLNATIVE:
                    _stack.push(callNativeFunction(bytecode()->getUInt16(_bcids.back() + 1)));
                    break;
                case BC_RETURN:
                    _vars.pop();
                    _functions.pop_back();
                    _bcids.pop_back();
                    break;
                default:
                    throw InterpreterException("Unknown bytecode");
            }

            static size_t lengths[] = {
    #define BYTECODE_SIZE(b, d, l) l,
                    FOR_BYTECODES(BYTECODE_SIZE)
    #undef BYTECODE_SIZE
            };

            if (!jmp)
                _bcids.back() += lengths[bytecode()->getInsn(_bcids.back())];
        }

        throw InterpreterException("Didn't found STOP bytecode");
    }

private:
    class Value {
    public:
        Value() : _type(VT_INVALID), _int(0) {}

        Value(int64_t value) : _type(VT_INT), _int(value) {}

        Value(double value) : _type(VT_DOUBLE), _double(value) {}

        Value(char const *value) : _type(VT_STRING), _string(value) {}

        Value(int64_t data, VarType type) : _type(type), _int(data) {}

        VarType type() const {
            return _type;
        }

        int64_t getInt() const {
            if (_type != VT_INT)
                throw InterpreterException("Type conflict");
            return _int;
        }

        double getDouble() const {
            if (_type != VT_DOUBLE)
                throw InterpreterException("Type conflict");
            return _double;
        }

        char const *getString() const {
            if (_type != VT_STRING)
                throw InterpreterException("Type conflict");
            return _string;
        }

    private:
        VarType _type;
        union {
            int64_t _int;
            double _double;
            char const *_string;
        };
    };

    class ScopeStorage {

    public:
        ScopeStorage(InterpreterCodeImpl *code) : _tmp(MIN_SIZE), _funids(MIN_SIZE) {
            size_t functionCount = 0;
            Code::FunctionIterator functionIterator(code);
            while (functionIterator.hasNext()) {
                ++functionCount;
                functionIterator.next();
            }
            _storage.resize(functionCount + 1);
        }

        void store(Value const &value, uint16_t ctx, uint16_t id) {
            std::vector<Value> *values = find(ctx);
            if (id >= values->size())
                values->resize(id + 1);
            (*values)[id] = value;
        }

        Value const load(uint16_t ctx, uint16_t id) {
            std::vector<Value> *values = find(ctx);
            if (id >= values->size() || (*values)[id].type() == VT_INVALID)
                throw InterpreterException("Undefined variable");
            return (*values)[id];
        }

        std::vector<Value> *find(uint16_t ctx) {
            if (ctx >= _storage.size())
                throw InterpreterException("Illegal context");
            if (ctx == 0)
                return &_tmp;
            return &_storage[ctx].back();
        }

        void push(uint16_t funid) {
            _funids.push_back(funid);
            _storage[funid].push_back(_tmp);
        }

        void pop() {
            _storage[_funids.back()].pop_back();
            _funids.pop_back();
        }

    private:
        static size_t const MIN_SIZE = 4;
        std::vector<Value> _tmp;
        std::vector<std::vector<std::vector<Value> > > _storage;
        std::vector<uint16_t> _funids;
    };

    void write(std::ostream &out, Interpreter::Value const &value, VarType type) {
        switch (type) {
            case VT_INT:
                out << value.getInt();
                break;
            case VT_DOUBLE:
                out << value.getDouble();
                break;
            case VT_STRING:
                out << value.getString();
                break;
            default:
                throw InterpreterException("Non-printable value");
        }
    }

    Value const callNativeFunction(uint16_t fid) {
        throw InterpreterException("Native calls aren't supported");
    }

    Bytecode *bytecode() {
        return _functions.back()->bytecode();
    }

    Value const popValue() {
        if (_stack.empty())
            throw InterpreterException("Can't get value from empty stack");
        Value const value(_stack.top());
        _stack.pop();
        return value;
    }

private:
    InterpreterCodeImpl *_code;
    ScopeStorage _vars;
    std::stack<Value> _stack;
    std::vector<BytecodeFunction *> _functions;
    std::vector<size_t> _bcids;
};


Status *InterpreterCodeImpl::execute(vector<Var *> &) {
    try {
        Interpreter(this).execute();
    } catch (InterpreterException const &e) {
        return Status::Error(e.what());
    }
    return Status::Ok();
}
}
