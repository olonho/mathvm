#pragma once
#include "mathvm.h"

#include <map>
#include <string>
#include <stack>
#include <vector>
#include <exception>
#include <iostream>

namespace mathvm { 

class StackVar : public Var {
public:
    StackVar() :
        Var(VT_INT, "")
    { }

    StackVar(VarType type) :
        Var(type, "")
    { }

    StackVar(VarType type, const std::string& name) :
        Var(type, name)
    { }

    static StackVar getFrom(int64_t i) {
        StackVar var(VT_INT);
        var.setIntValue(i);
        return var;
    }
    static StackVar getFrom(double d) {
        StackVar var(VT_DOUBLE);
        var.setDoubleValue(d);
        return var;
    }
    static StackVar getFrom(const char * value) {
        StackVar var(VT_STRING);
        var.setStringValue(value);
        return var;
    }
};

class InterprContext {
    BytecodeFunction* bFunc;
    InterprContext* parent;
    std::vector<StackVar> variables;
    uint32_t bci_;

public:
    InterprContext(BytecodeFunction* bFunc, InterprContext* parent = NULL) :
            bFunc(bFunc), parent(parent), variables(bFunc->localsNumber()), bci_(0) {
    }

    Instruction nextInsn() {
        return bc()->getInsn(bci_++);
    }

    int64_t getInt() {
        int64_t res = bc()->getInt64(bci_);
        bci_ += sizeof(int64_t);
        return res;
    }

    uint16_t getUInt16() {
        uint16_t res = bc()->getUInt16(bci_);
        bci_ += sizeof(uint16_t);
        return res;
    }

    int16_t getInt16() {
        int16_t res = bc()->getInt16(bci_);
        bci_ += sizeof(int16_t);
        return res;
    }

    double getDouble() {
        double res = bc()->getDouble(bci_);
        bci_ += sizeof(double);
        return res;
    }

    void jump() {
        int16_t offset = getInt16();
        bci_ += offset - sizeof(int16_t);
    }

    Bytecode* bc() {
        return bFunc->bytecode();
    }

    uint32_t bci() {
        return bci_;
    }

    InterprContext* getParent() {
        return parent;
    }

    StackVar getVar() {
        uint16_t  var_id = getUInt16();
        if (variables.size() < var_id) {
            throw new std::runtime_error("Unknown var");
        }
        return variables[var_id];
    }

    StackVar getVarById(uint16_t var_id) {
        if (variables.size() < var_id) {
            throw new std::runtime_error("Unknown var");
        }
        return variables[var_id];
    }

    void storeVar(StackVar var) {
        uint16_t var_id = getUInt16();
        if (variables.size() < var_id) {
            throw new std::runtime_error("Unknown var");
        }
        variables[var_id] = var;
    }

    void storeVarById(StackVar var, uint16_t var_id) {
        if (variables.size() < var_id) {
            throw new std::runtime_error("Unknown var");
        }
        variables[var_id] = var;
    }

    StackVar getCTXVar() {
        uint16_t contextid = getUInt16();
        uint16_t var_id = getUInt16();
        return getCTXVar(contextid, var_id);
    }

    StackVar getCTXVar(uint16_t contextid, uint16_t var_id) {
        if (contextid == bFunc->scopeId()) {
            if (variables.size() < var_id) {
                throw new std::runtime_error("Unknown var");
            }
            return variables[var_id];
        }
        if (parent != NULL) {
            return parent->getCTXVar(contextid, var_id);
        }
        throw new std::runtime_error("Unknown var");
    }

    void storeCTXVar(StackVar var) {
        uint16_t contextid = getUInt16();
        uint16_t var_id = getUInt16();
        storeCTXVar(var, contextid, var_id);
    }

    void storeCTXVar(StackVar var, uint16_t contextid, uint16_t var_id) {
        if (contextid == bFunc->scopeId()) {
            if (variables.size() < var_id) {
                throw new runtime_error("Unknown var");
            }
            variables[var_id] = var;
        }
        if (parent != NULL) {
            parent->storeCTXVar(var, contextid, var_id);
        }
        throw new std::runtime_error("Unknown var");
    }
};

class InterpreterCodeImpl : public Code {
private:
    InterprContext* context;
    std::vector<StackVar> stack;
public:
    InterpreterCodeImpl() :
        context(NULL) {
    }

    ~InterpreterCodeImpl() {
        if (context != NULL) {
            delete context;
        }
    }

    Status* execute(vector<Var*>&);
private:
    bool executeInsn(Instruction ins);
    Bytecode* bc() {
        return context->bc();
    }
};
}