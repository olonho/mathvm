#include <dlfcn.h>
#include <cmath>

#include "code_interpreter.h"
#include "interpreter_exception.h"

using namespace mathvm;

CodeInterpreter::CodeInterpreter() 
{}

CodeInterpreter::~CodeInterpreter() 
{
    if (_context)
    {
        delete _context;
    }
}

Status* CodeInterpreter::execute(vector<Var*>& vars) 
{
    BytecodeFunction* top = (BytecodeFunction*) functionById(0);
    try 
    {
        for (_context = new InterpreterContext(top); _context->hasNextInstruction(); )
            executeInstruction(_context->getInstruction());
    } 
    catch (InterpreterException e) 
    {
        return Status::Error(e.what(), _context->getPosition());
    }
    
    return Status::Ok();
}

void CodeInterpreter::executeInstruction(Instruction instruction) 
{
    if (tryLoad(instruction))
        return;
    if (tryArithmetic(instruction))
        return;
    if (tryLogic(instruction))
        return;
    if (tryPrint(instruction))
        return;
    if (tryConvert(instruction))
        return;
    
    throw InterpreterException("Invalid instruction on execution step!");
}

bool CodeInterpreter::tryLoad(Instruction instruction) 
{
    switch (instruction)
    {
        case BC_DLOAD:
            _stack.push(_context->getDouble());
            break;
        case BC_ILOAD:
            _stack.push(_context->getInt64());
            break;
        case BC_SLOAD:
            _stack.push(_context->getUInt16());
            break;
        case BC_DLOAD0:
            _stack.push((double) 0.0);
            break;
        case BC_ILOAD0:
            _stack.push((int64_t) 0);
            break;
        case BC_SLOAD0:
            _stack.push(makeStringConstant(""));
            break;
        case BC_DLOAD1:
            _stack.push((double) 1.0);
            break;
        case BC_ILOAD1:
            _stack.push((int64_t) 1);
            break;
        case BC_DLOADM1:
            _stack.push((double) -1.0);
            break;
        case BC_ILOADM1:
            _stack.push((int64_t) -1);
            break;
        case BC_LOADIVAR0:
        case BC_LOADDVAR0:
        case BC_LOADSVAR0:
            _stack.push(_context->getVariableById(0));
            break;
        case BC_LOADIVAR1:
        case BC_LOADDVAR1:
        case BC_LOADSVAR1:
            _stack.push(_context->getVariableById(1));
            break;
        case BC_LOADIVAR2:
        case BC_LOADDVAR2:
        case BC_LOADSVAR2:
            _stack.push(_context->getVariableById(2));
            break;
        case BC_LOADIVAR3:
        case BC_LOADDVAR3:
        case BC_LOADSVAR3:
            _stack.push(_context->getVariableById(3));
            break;
        default:
            // not load instruction
            return false;
    }
    
    // executed  instruction
    return true;
}

bool CodeInterpreter::tryArithmetic(Instruction instruction) 
{
    switch (instruction)
    {
        case BC_DADD:
            _stack.push(popStack().getDouble() + popStack().getDouble());
            break;
        case BC_IADD:
            _stack.push(popStack().getInt64() + popStack().getInt64());
            break;
        case BC_DSUB:
            _stack.push(popStack().getDouble() - popStack().getDouble());
            break;
        case BC_ISUB: 
            _stack.push(popStack().getInt64() - popStack().getInt64());
            break;
        case BC_DMUL:
            _stack.push(popStack().getDouble() * popStack().getDouble());
            break;
        case BC_IMUL:
            _stack.push(popStack().getInt64() * popStack().getInt64());
            break;
        case BC_DDIV:
            _stack.push(popStack().getDouble() / popStack().getDouble());
            break;
        case BC_IDIV:
            _stack.push(popStack().getInt64() / popStack().getInt64());
            break;
        case BC_IMOD:
            _stack.push(popStack().getInt64() % popStack().getInt64());
            break;
        case BC_DNEG:
            _stack.push(-popStack().getDouble());
            break;
        case BC_INEG: 
            _stack.push(-popStack().getInt64());
            break;
        default:
            return false;
    }
    
    return true;
}

bool CodeInterpreter::tryLogic(Instruction instruction) 
{
    switch (instruction)
    {
        case BC_IAOR:
            _stack.push(popStack().getInt64() | popStack().getInt64());
            break;
        case BC_IAAND:
            _stack.push(popStack().getInt64() & popStack().getInt64());
            break;
        case BC_IAXOR:
            _stack.push(popStack().getInt64() ^ popStack().getInt64());
            break;
        default:
            return false;
    }
    
    return true;
}

bool CodeInterpreter::tryPrint(Instruction instruction) 
{    
    switch (instruction)
    {
        case BC_DPRINT:
            std::cout << popStack().getDouble();
            break;
        case BC_IPRINT:
            std::cout << popStack().getInt64();
            break;
        case BC_SPRINT:
            std::cout << constantById(popStack().getUint16());
            break;
        default:
            return false;
    }
    
    return true;
}

bool CodeInterpreter::tryConvert(Instruction instruction) 
{
    switch (instruction)
    {        
        case BC_I2D:
        {
            int64_t intData = popStack().getInt64();
            _stack.push((double) intData);
            break;
        }
        case BC_D2I:
        {
            double dobuleData = popStack().getDouble();
            _stack.push(std::round(dobuleData));
            break;
        }
        case BC_S2I: 
        {
            uint16_t value = popStack().getUint16();
            _stack.push((int64_t) value);
            break;
        }
        default:
            return false;   
    }
    
    return true;
}



StackElement CodeInterpreter::popStack() 
{
    const auto item = _stack.top();
    _stack.pop();
    
    return item;
}

