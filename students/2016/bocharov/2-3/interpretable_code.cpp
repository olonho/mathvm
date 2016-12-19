#include "interpretable_code.h"

namespace mathvm
{

namespace
{

/* see ctor of Code */
constexpr uint16_t EMPTY_STR_ID = 0;

/* crunch to allow use uninitialized vars */
constexpr int NULL_VALUE_TYPE = VT_INVALID - 1;

}   // namespace


Value::Value()
    : m_type(VT_INVALID)
{}

Value::Value(int64_t i)
    : m_type(VT_INT)
{
    m_val.i = i;
}

Value::Value(double d)
    : m_type(VT_DOUBLE)
{
    m_val.d = d;
}

Value::Value(uint16_t strId)
    : m_type(VT_STRING)
{
    m_val.s = strId;
}

Value Value::nullValue()
{
    Value v;
    v.m_val.i = 0;
    v.m_type = (VarType) NULL_VALUE_TYPE;
    return v;
}


int64_t Value::asInt() const
{
    checkType(VT_INT);
    return m_val.i;
}

double Value::asDouble() const
{
    checkType(VT_DOUBLE);
    return m_val.d;
}

uint16_t Value::asStrId() const
{
    checkType(VT_STRING);
    return m_val.s;
}

void Value::checkType(VarType type) const
{
    if (type != m_type && (VarType) NULL_VALUE_TYPE != m_type) {
        auto message = std::string("types differ: expected ") + typeToName(type) +
                " got " + typeToName(m_type);
        throw ExecutionError(message);
    }
}


Context::Context(BytecodeFunction * function, ContextPtr parent)
    : m_function(function)
    , m_bcPos(0)
    , m_parent(parent)
{}

ContextPtr Context::getParent()
{
    return m_parent;
}

uint16_t Context::getId()
{
    return m_function->scopeId();
}

bool Context::hasBc() const
{
    return m_bcPos < m_function->bytecode()->length();
}

Instruction Context::nextBc()
{
    Instruction bc = m_function->bytecode()->getInsn(m_bcPos);
    m_bcPos += 1;
    return bc;
}

void Context::goTo(int16_t off)
{
    m_bcPos = m_bcPos + off - sizeof(off);
    if (m_bcPos > m_function->bytecode()->length())
        throw ExecutionError("jump out of function scope");
}

int64_t Context::nextInt()
{
    auto v = m_function->bytecode()->getInt64(m_bcPos);
    m_bcPos += sizeof(v);
    return v;
}

double Context::nextDouble()
{
    auto v = m_function->bytecode()->getDouble(m_bcPos);
    m_bcPos += sizeof(v);
    return v;
}

uint16_t Context::nextUInt16()
{
    auto v = m_function->bytecode()->getUInt16(m_bcPos);
    m_bcPos += sizeof(v);
    return v;
}

int16_t Context::nextInt16()
{
    auto v = m_function->bytecode()->getInt16(m_bcPos);
    m_bcPos += sizeof(v);
    return v;
}

Value Context::getVar(uint16_t id)
{
    checkVarInScope(id);
    auto it = m_vars.find(id);
    if (it == m_vars.end()) {
        m_vars[id] = Value::nullValue();
    }
    return m_vars[id];
}

void Context::setVar(uint16_t id, Value const & v)
{
    checkVarInScope(id);
    m_vars[id] = v;
}

void Context::checkVarInScope(uint16_t id)
{
    if (id >= m_function->localsNumber())
        throw ExecutionError("var with id = " + std::to_string(id) +
            "is out of scope ctx " + std::to_string(getId()));
}

Status* InterpretableCode::execute(vector<Var*>& vars)
{
    BytecodeFunction * currentFunction = dynamic_cast<BytecodeFunction *>(functionById(0));
    assert(currentFunction);
    m_currentCtx = std::make_shared<Context>(currentFunction, nullptr);
    m_stopped = false;
    m_contexts[currentFunction->scopeId()].push(m_currentCtx);
    while (m_currentCtx->hasBc() && !m_stopped) {
        execInsn(m_currentCtx->nextBc());
    }

    return Status::Ok();
}

void InterpretableCode::execInsn(Instruction insn)
{
    switch (insn) {
    // loads
    case BC_DLOAD:
        push(m_currentCtx->nextDouble());
        break;
    case BC_ILOAD:
        push(m_currentCtx->nextInt());
        break;
    case BC_SLOAD:
        push(m_currentCtx->nextUInt16());
        break;
    case BC_DLOAD0:
        push(0.);
        break;
    case BC_ILOAD0:
        push(0L);
        break;
    case BC_SLOAD0:
        push(EMPTY_STR_ID);
        break;
    case BC_DLOAD1:
        push(1.);
        break;
    case BC_ILOAD1:
        push(1L);
        break;
    case BC_DLOADM1:
        push(-1.);
        break;
    case BC_ILOADM1:
        push(-1L);
        break;
    case BC_LOADDVAR0:
    case BC_LOADIVAR0:
    case BC_LOADSVAR0:
        load(0);
        break;
    case BC_LOADDVAR1:
    case BC_LOADIVAR1:
    case BC_LOADSVAR1:
        load(1);
        break;
    case BC_LOADDVAR2:
    case BC_LOADIVAR2:
    case BC_LOADSVAR2:
        load(2);
        break;
    case BC_LOADDVAR3:
    case BC_LOADIVAR3:
    case BC_LOADSVAR3:
        load(3);
        break;
    case BC_LOADDVAR:
    case BC_LOADIVAR:
    case BC_LOADSVAR:
        load(m_currentCtx->nextUInt16());
        break;
    case BC_LOADCTXDVAR:
    case BC_LOADCTXIVAR:
    case BC_LOADCTXSVAR: {
        auto ctx = m_currentCtx->nextUInt16();
        auto var = m_currentCtx->nextUInt16();
        load(ctx, var);
        break;
    }
    // stores
    case BC_STOREDVAR0:
    case BC_STOREIVAR0:
    case BC_STORESVAR0:
        store(0);
        break;
    case BC_STOREDVAR1:
    case BC_STOREIVAR1:
    case BC_STORESVAR1:
        store(1);
        break;
    case BC_STOREDVAR2:
    case BC_STOREIVAR2:
    case BC_STORESVAR2:
        store(2);
        break;
    case BC_STOREDVAR3:
    case BC_STOREIVAR3:
    case BC_STORESVAR3:
        store(3);
        break;
    case BC_STOREDVAR:
    case BC_STOREIVAR:
    case BC_STORESVAR:
        store(m_currentCtx->nextUInt16());
        break;
    case BC_STORECTXDVAR:
    case BC_STORECTXIVAR:
    case BC_STORECTXSVAR: {
        auto ctx = m_currentCtx->nextUInt16();
        auto var = m_currentCtx->nextUInt16();
        store(ctx, var);
        break;
    }
    // arithmetic
    case BC_DADD:
        push(pop().asDouble() + pop().asDouble());
        break;
    case BC_IADD:
        push(pop().asInt() + pop().asInt());
        break;
    case BC_DSUB:
        push(pop().asDouble() - pop().asDouble());
        break;
    case BC_ISUB:
        push(pop().asInt() - pop().asInt());
        break;
    case BC_DMUL:
        push(pop().asDouble() * pop().asDouble());
        break;
    case BC_IMUL:
        push(pop().asInt() * pop().asInt());
        break;
    case BC_DDIV:
        push(pop().asDouble() / pop().asDouble());
        break;
    case BC_IDIV:
        push(pop().asInt() / pop().asInt());
        break;
    case BC_IMOD:
        push(pop().asInt() % pop().asInt());
        break;
    case BC_DNEG:
        push(-pop().asDouble());
        break;
    case BC_INEG:
        push(-pop().asInt());
        break;
    case BC_IAOR:
        push(pop().asInt() | pop().asInt());
        break;
    case BC_IAAND:
        push(pop().asInt() & pop().asInt());
        break;
    case BC_IAXOR:
        push(pop().asInt() ^ pop().asInt());
        break;
    // prints
    case BC_IPRINT:
        std::cout << pop().asInt();
        break;
    case BC_DPRINT:
        std::cout << pop().asDouble();
        break;
    case BC_SPRINT:
        std::cout << constantById(pop().asStrId());
        break;
    // conversions
    case BC_I2D:
        push((double) pop().asInt());
        break;
    case BC_D2I:
        push((int64_t) pop().asDouble());
        break;
    case BC_S2I:
        push((int64_t) constantById(pop().asStrId()).c_str());
        break;

    // stack manips
    case BC_SWAP: {
        auto v1 = pop();
        auto v2 = pop();
        push(v1);
        push(v2);
        break;
    }
    case BC_POP:
        pop();
        break;
    // comparisons
    case BC_DCMP: {
        auto upper = pop().asDouble();
        auto lower = pop().asDouble();
        int64_t result = upper < lower ? -1 : (upper == lower ? 0 : 1);
        push(result);
        break;
    }
    case BC_ICMP: {
        auto upper = pop().asInt();
        auto lower = pop().asInt();
        int64_t result = upper < lower ? -1 : (upper == lower ? 0 : 1);
        push(result);
        break;
    }
    case BC_JA:
        m_currentCtx->goTo(m_currentCtx->nextInt16());
        break;
    case BC_IFICMPNE: {
        auto off = m_currentCtx->nextInt16();
        if (pop().asInt() != pop().asInt()) m_currentCtx->goTo(off);
        break;
    }
    case BC_IFICMPE: {
        auto off = m_currentCtx->nextInt16();
        if (pop().asInt() == pop().asInt()) m_currentCtx->goTo(off);
        break;
    }
    case BC_IFICMPG: {
        auto off = m_currentCtx->nextInt16();
        if (pop().asInt() > pop().asInt()) m_currentCtx->goTo(off);
        break;
    }
    case BC_IFICMPGE: {
        auto off = m_currentCtx->nextInt16();
        if (pop().asInt() >= pop().asInt()) m_currentCtx->goTo(off);
        break;
    }
    case BC_IFICMPL: {
        auto off = m_currentCtx->nextInt16();
        if (pop().asInt() < pop().asInt()) m_currentCtx->goTo(off);
        break;
    }
    case BC_IFICMPLE: {
        auto off = m_currentCtx->nextInt16();
        if (pop().asInt() <= pop().asInt()) m_currentCtx->goTo(off);
        break;
    }
    case BC_DUMP:
        assert(false && "TODO");
        break;
    case BC_STOP:
        m_stopped = true;
        return;
    // function related
    case BC_CALL: {
        auto id = m_currentCtx->nextUInt16();
        auto function = (BytecodeFunction *) functionById(id);
        m_currentCtx = make_shared<Context>(function, m_currentCtx);
        m_contexts[id].push(m_currentCtx);
        break;
    }
    case BC_CALLNATIVE:
        assert(false && "not implemented");
        break;
    case BC_RETURN: {
        m_contexts[m_currentCtx->getId()].pop();
        m_currentCtx = m_currentCtx->getParent();
        break;
    }
    default: {
        auto message = std::string("unsupported instruction") + bytecodeName(insn, nullptr);
        throw ExecutionError(message);
    }
    }
}

Value InterpretableCode::pop()
{
    auto top = m_stack.top();
    m_stack.pop();
    return top;
}

void InterpretableCode::push(Value const & value)
{
    m_stack.push(value);
}

void InterpretableCode::load(uint16_t var)
{
    push(m_currentCtx->getVar(var));
}

void InterpretableCode::load(uint16_t ctx, uint16_t var)
{
    push(findCtx(ctx)->getVar(var));
}

void InterpretableCode::store(uint16_t var)
{
    auto v = pop();
    m_currentCtx->setVar(var, v);
}

void InterpretableCode::store(uint16_t ctx, uint16_t var)
{
    auto v = pop();
    findCtx(ctx)->setVar(var, v);
}

ContextPtr InterpretableCode::findCtx(uint16_t ctx)
{
    auto it = m_contexts.find(ctx);
    if (it == m_contexts.end()) {
        throw ExecutionError("unknown context with id = " + std::to_string(ctx));
    }
    return it->second.top();
}

}   // namespace mathvm
