#ifndef M_INTERPTER_H
#define M_INTERPTER_H

#include <vector>

#include "interpreter_error.h"
#include "mathvm.h"

namespace mathvm {

class stack_var {
    union {
        int64_t _i;
        double _d;
        uint16_t _u;
    } _data;
    VarType _type;

public:
    stack_var() :
        _type(VT_INVALID) {

    }
    static stack_var create(int64_t i) {
        stack_var var;
        var._data._i = i;
        var._type = VT_INT;
        return var;
    }
    static stack_var create(double d) {
        stack_var var;
        var._data._d = d;
        var._type = VT_DOUBLE;
        return var;
    }
    static stack_var create(uint16_t u) {
        stack_var var;
        var._data._u = u;
        var._type = VT_STRING;
        return var;
    }
    int64_t  getAsInt() {
        return _data._i;
    }
    double getAsDouble() {
        return _data._d;
    }
    uint16_t getAsUInt16() {
        return _data._u;
    }

};

class InterpreterScopeContext {
    BytecodeFunction* _byte_func;
    std::vector<stack_var> _scope_vars;
    uint32_t _bci;
    InterpreterScopeContext* _parent;


public:
    InterpreterScopeContext(BytecodeFunction* bf, InterpreterScopeContext* parent = nullptr) :
        _byte_func(bf),
        _scope_vars(bf->localsNumber()),
        _bci(0),
        _parent(parent) {
    }

    ~InterpreterScopeContext() {
    }


    Instruction getInsn() {
        return bc()->getInsn(_bci++);
    }

    int64_t getInt() {
        int64_t res = bc()->getInt64(_bci);
        _bci += sizeof(int64_t);
        return res;
    }

    uint16_t getUInt16() {
        uint16_t res = bc()->getUInt16(_bci);
        _bci += sizeof(uint16_t);
        return res;
    }

    int16_t getInt16() {
        int16_t res = bc()->getInt16(_bci);
        _bci += sizeof(int16_t);
        return res;
    }

    double getDouble() {
        double res = bc()->getDouble(_bci);
        _bci += sizeof(double);
        return res;
    }

    stack_var getVar() {
        uint16_t  var_id = getUInt16();
        if (_scope_vars.size() < var_id) {
            throw InterpreterError("Invalid var");
        }
        return _scope_vars[var_id];
    }

    stack_var getVarById(uint16_t var_id) {
        if (_scope_vars.size() < var_id) {
            throw InterpreterError("Invalid var");
        }
        return _scope_vars[var_id];
    }

    void storeVar(stack_var var) {
        uint16_t  var_id = getUInt16();
        if (_scope_vars.size() < var_id) {
            throw InterpreterError("Invalid var");
        }
        _scope_vars[var_id] = var;
    }

    void storeVarById(stack_var var, uint16_t var_id) {
        if (_scope_vars.size() < var_id) {
            throw InterpreterError("Invalid var");
        }
        _scope_vars[var_id] = var;
    }

    stack_var getContextVar() {
        uint16_t context_id = getUInt16();
        uint16_t var_id = getUInt16();
        return getContextVar(context_id, var_id);
    }

    stack_var getContextVar(uint16_t context_id, uint16_t var_id) {
        if (context_id == _byte_func->scopeId()) {
            if (_scope_vars.size() < var_id) {
                throw InterpreterError("Invalid var");
            }
            return _scope_vars[var_id];
        } else if (_parent != nullptr) {
            return _parent->getContextVar(context_id, var_id);
        } else {
            throw InterpreterError("Invalid var");
        }
    }

    void storeContextVar(stack_var var) {
        uint16_t context_id = getUInt16();
        uint16_t  var_id = getUInt16();
        storeContextVar(var, context_id, var_id);
    }

    void storeContextVar(stack_var var, uint16_t context_id, uint16_t var_id) {
        if (context_id == _byte_func->scopeId()) {
            if (_scope_vars.size() < var_id) {
                throw InterpreterError("Invalid var");
            }
            _scope_vars[var_id] = var;
        } else if (_parent != nullptr) {
            _parent->storeContextVar(var, context_id, var_id);
        } else {
            throw InterpreterError("Invalid Var");
        }

    }

    void jmp() {
        int16_t offset = getInt16();
        _bci += offset - sizeof(int16_t);
    }

    Bytecode* bc() {
        return _byte_func->bytecode();
    }

    uint32_t bci() {
        return _bci;
    }

    InterpreterScopeContext* getParent() {
        return _parent;
    }


};


class InterpreterCode : public Code {
    InterpreterScopeContext* _context;
    std::vector<stack_var> _stack;

public:
    InterpreterCode() :
        _context(nullptr) {
    }
    ~InterpreterCode() {
        if (_context != nullptr) {
            delete _context;
        }
    }
    Status* execute(std::vector<Var*> &vars);
private:
    bool executeInstruction(Instruction insn);
    std::pair<stack_var, stack_var> getOperandsForBinOp();

    Bytecode* bc() {
        return _context->bc();
    }
};
}

#endif //M_INTERPTER_H
