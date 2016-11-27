#ifndef INTERPRETABLE_CODE_H
#define INTERPRETABLE_CODE_H

#include "mathvm.h"

#include <memory>
#include <stdexcept>
#include <stack>
#include <unordered_map>

namespace mathvm
{

class ExecutionError : public std::runtime_error
{
public:
    ExecutionError(std::string const & msg = "")
        : std::runtime_error(msg)
    {}
};

class Value
{
public:
    Value();
    Value(int64_t i);
    Value(double d);
    Value(uint16_t strId);

    static Value nullValue();

    int64_t asInt() const;
    double asDouble() const;
    uint16_t asStrId() const;

private:
    void checkType(VarType type) const;

    union {
        int64_t i;
        double d;
        uint16_t s;
    } m_val;
    VarType m_type;
};
using ValueMap = std::unordered_map<uint16_t, Value>;

class Context;
using ContextPtr = std::shared_ptr<Context>;

class Context
{
public:
    Context(BytecodeFunction * function, ContextPtr parent);

    ContextPtr getParent();
    uint16_t getId();

    Instruction nextBc();
    bool hasBc() const;
    void goTo(int16_t off);

    uint16_t nextUInt16();
    int16_t nextInt16();
    int64_t nextInt();
    double nextDouble();

    Value getVar(uint16_t id);
    void setVar(uint16_t id, Value const & v);

private:
    void checkVarInScope(uint16_t id);

private:
    BytecodeFunction * m_function;
    uint32_t m_bcPos;
    ContextPtr m_parent;
    ValueMap m_vars;
};
using ContextMap = std::unordered_map<uint16_t, std::stack<ContextPtr>>;

class InterpretableCode : public Code
{
public:
    Status* execute(vector<Var*>& vars) override;

private:
    void execInsn(Instruction insn);

    Value pop();
    void push(Value const & value);

    void load(uint16_t var);
    void load(uint16_t ctx, uint16_t var);

    void store(uint16_t var);
    void store(uint16_t ctx, uint16_t var);

    ContextPtr findCtx(uint16_t ctx);

private:
    ContextPtr m_currentCtx;
    std::stack<Value, std::vector<Value>> m_stack;
    bool m_stopped;
    ContextMap m_contexts;
};

}   // namespace mathvm

#endif  // INTERPRETABLE_CODE_H
