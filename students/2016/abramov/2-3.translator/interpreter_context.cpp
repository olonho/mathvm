#include <dlfcn.h>
#include <cmath>

#include "interpreter_context.h"
#include "interpreter_exception.h"

using namespace mathvm;

StackElement::StackElement() 
    : _type(VT_INVALID)
{}

StackElement::StackElement(int64_t value) 
    : _type(VT_INT)
{
    _value.int64Value = value;
}

StackElement::StackElement(double value)
    : _type(VT_DOUBLE)
{
    _value.doubleValue = value;
}

StackElement::StackElement(uint16_t value)
    : _type(VT_STRING)
{
    _value.uint16Value = value;
}

int64_t StackElement::getInt64() const
{
    testType(VT_INT);
    return _value.int64Value;
}

double StackElement::getDouble() const
{
    testType(VT_DOUBLE);
    return _value.doubleValue;
}

uint16_t StackElement::getUint16() const
{
    testType(VT_STRING);
    return _value.uint16Value;
}

void StackElement::testType(VarType type) const 
{
    if (type == VT_INVALID)
    {
        throw InterpreterException("VT_INVALID type on test type");
    }
    
    if (type != _type)
    {
        throw InterpreterException("Type mismatch on test type");
    }
}

InterpreterContext::InterpreterContext(BytecodeFunction* func, InterpreterContext* parent)
    : _func(func)
    , _parent(parent)
    , _vars(func->localsNumber())
    , _pos(0)
{}

InterpreterContext::InterpreterContext(const InterpreterContext& other)
    : _func(other._func)
    , _parent(other._parent)
    , _vars(other._vars)
    , _pos(other._pos)
{}

InterpreterContext::~InterpreterContext() 
{
}

Bytecode* InterpreterContext::getBytecode() 
{
    return _func->bytecode();
}

uint32_t InterpreterContext::getPosition() const 
{
    return _pos;
}

double InterpreterContext::getDouble() 
{
    double output = getBytecode()->getDouble(_pos);
    _pos += sizeof(output);
    
    return output;
}

int64_t InterpreterContext::getInt64() 
{
    int64_t output = getBytecode()->getInt64(_pos);
    _pos += sizeof(output);
    
    return output;
}

uint16_t InterpreterContext::getUInt16() 
{
    uint16_t output = getBytecode()->getUInt16(_pos);
    _pos += sizeof(output);
    
    return output;
}

Instruction InterpreterContext::getInstruction()
{
    return getBytecode()->getInsn(_pos++);
}

StackElement InterpreterContext::getVariableById(uint16_t index) const 
{
    checkAccess(index);
    return _vars[index];
}


bool InterpreterContext::hasNextInstruction() 
{
    return _pos < getBytecode()->length();
}

void InterpreterContext::checkAccess(uint16_t index) const
{
    if (_vars.size() < index) 
        throw InterpreterException("Index is out of context bound!");
}








