#ifndef INTERPRETER_H
#define INTERPRETER_H

#include <vector>
#include <stack>

#include "mathvm.h"
#include "interpreter_exception.h"

namespace mathvm {

class StackValue {
    union {
        int64_t i_;
        double d_;
        uint16_t u_;
    } value_;
    VarType type_;

public:
    StackValue() :
            type_(VT_INVALID) {}

    StackValue(int64_t i) :
            type_(VT_INT) {
        value_.i_ = i;
    }

    StackValue(double d) :
            type_(VT_DOUBLE) {
        value_.d_ = d;
    }

    StackValue(uint16_t u) :
            type_(VT_STRING) {
        value_.u_ = u;
    }

    int64_t getInt() const {
        checkType(VT_INT);
        return value_.i_;
    }

    double getDouble() const {
        checkType(VT_DOUBLE);
        return value_.d_;
    }

    uint16_t getUint16() const {
        checkType(VT_STRING);
        return value_.u_;
    }

    void checkType(VarType type) const {
        if (type_ != type) {
            throw InterpreterException("Type mismatch");
        }
    }
};

class InterpreterScope {
public:
    InterpreterScope(BytecodeFunction* function, InterpreterScope* parent = NULL) :
            function_(function),
            parent_(parent),
            scopeVars_(function->localsNumber()),
            position_(0) {}

    ~InterpreterScope() {
    }

    Instruction getInsn() {
        return bytecode()->getInsn(position_++);
    }

    int64_t getInt() {
        int64_t res = bytecode()->getInt64(position_);
        position_ += sizeof(int64_t);
        return res;
    }

    uint16_t getUInt16() {
        uint16_t res = bytecode()->getUInt16(position_);
        position_ += sizeof(uint16_t);
        return res;
    }

    int16_t getInt16() {
        int16_t res = bytecode()->getInt16(position_);
        position_ += sizeof(int16_t);
        return res;
    }

    double getDouble() {
        double res = bytecode()->getDouble(position_);
        position_ += sizeof(double);
        return res;
    }

    StackValue getVar() {
        return getVarById(getUInt16());
    }

    StackValue getVarById(uint16_t id) {
        checkBounds(id);
        return scopeVars_[id];
    }

    void storeVar(StackValue value) {
        storeVarById(value, getUInt16());
    }

    void storeVarById(StackValue var, uint16_t id) {
        checkBounds(id);
        scopeVars_[id] = var;
    }

    StackValue getContextVar() {
        uint16_t contextId = getUInt16();
        uint16_t varId = getUInt16();
        return getContextVar(contextId, varId);
    }

    StackValue getContextVar(uint16_t contextId, uint16_t varId) {
        if (contextId == function_->scopeId()) {
            checkBounds(varId);
            return scopeVars_[varId];
        }
        if (parent_ != NULL) {
            return parent_->getContextVar(contextId, varId);
        }
        throw InterpreterException("Invalid variable");
    }

    void storeContextVar(StackValue var) {
        uint16_t scopeId = getUInt16();
        uint16_t varId = getUInt16();
        storeContextVar(var, scopeId, varId);
    }

    void storeContextVar(StackValue var, uint16_t scopeId, uint16_t varId) {
        if (scopeId == function_->scopeId()) {
            checkBounds(varId);
            scopeVars_[varId] = var;
        } else if (parent_ != NULL) {
            parent_->storeContextVar(var, scopeId, varId);
        } else {
            throw InterpreterException("Invalid variable to store");
        }

    }

    void jump(bool condition) {
        if (condition) {
            int16_t offset = getInt16();
            position_ += offset - sizeof(int16_t);
        } else {
            getInt16();
        }
    }

    Bytecode* bytecode() const {
        return function_->bytecode();
    }

    bool hasNextInsn() const {
        return position_ < bytecode()->length();
    }

    InterpreterScope* getParent() const {
        return parent_;
    }

    uint32_t bytecodePosition() const {
        return position_;
    }

private:
    BytecodeFunction* function_;
    InterpreterScope* parent_;
    vector<StackValue> scopeVars_;
    uint32_t position_;

    void checkBounds(uint16_t id) const {
        if (scopeVars_.size() < id) {
            throw InterpreterException("Variable id out of bounds");
        }
    }
};

class InterpreterCodeImpl : public Code {
public:
    InterpreterCodeImpl() :
        scope_(NULL) {}

    ~InterpreterCodeImpl() {
        if (scope_ != NULL) {
            delete scope_;
        }
    }

    virtual Status* execute(vector<Var*>& vars) override {
        try {
            BytecodeFunction* top = (BytecodeFunction*) functionById(0);
            scope_ = new InterpreterScope(top, NULL);
            while (scope_->hasNextInsn()) {
                if (execute(scope_->getInsn())) {
                    break;
                }
            }
        } catch (InterpreterException e) {
            return Status::Error(e.what(), scope_->bytecodePosition());
        }
        return Status::Ok();
    }

private:
    InterpreterScope* scope_;
    stack<StackValue> stack_;

    bool execute(Instruction insn);

    StackValue loadTos();

    void swapTos();
};

}
#endif // INTERPRETER_H