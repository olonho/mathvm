#include <dlfcn.h>
#include <cmath>

#include "code_interpreter.h"
#include "interpreter_exception.h"
#include "generator_context.h"

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
    if (tryStore(instruction))
        return;
    if (tryArithmetic(instruction))
        return;
    if (tryLogic(instruction))
        return;
    if (tryPrint(instruction))
        return;
    if (tryConvert(instruction))
        return;
    if (tryCompare(instruction))
        return;
    if (tryOther(instruction))
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
        case BC_LOADCTXIVAR:
        case BC_LOADCTXDVAR:
        case BC_LOADCTXSVAR:
        {
            uint16_t contextId = _context->getUInt16();
            uint16_t varId = _context->getUInt16();
            _stack.push(_context->getContextVariable(contextId, varId));
            break;
        }
        default:
            // not load instruction
            return false;
    }
    
    // executed  instruction
    return true;
}

bool CodeInterpreter::tryStore(Instruction instruction) 
{
    switch (instruction)
    {
        case BC_STOREIVAR0:
        case BC_STOREDVAR0:
        case BC_STORESVAR0:
            _context->storeVariableById(popStack(), 0);
            break;
        case BC_STOREIVAR1:
        case BC_STOREDVAR1:
        case BC_STORESVAR1:
            _context->storeVariableById(popStack(), 1);
            break;
        case BC_STOREIVAR2:
        case BC_STOREDVAR2:
        case BC_STORESVAR2:
            _context->storeVariableById(popStack(), 2);
            break;
        case BC_STOREIVAR3:
        case BC_STOREDVAR3:
        case BC_STORESVAR3:
            _context->storeVariableById(popStack(), 3);
            break;
        case BC_STOREIVAR:
        case BC_STOREDVAR:
        case BC_STORESVAR:
        {
            uint16_t index = _context->getUInt16();
            _context->storeVariableById(popStack(), index);
            break;
        }
        case BC_STORECTXIVAR:
        case BC_STORECTXDVAR:
        case BC_STORECTXSVAR:
        {
            uint16_t contextId = _context->getUInt16();
            uint16_t varId = _context->getUInt16();
            _context->storeContextVariable(popStack(), contextId, varId);
            break;
        }
        default:
            return false;
    }
    
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

bool CodeInterpreter::tryCompare(Instruction instruction) 
{
    switch (instruction)
    {
        case BC_ICMP: 
        {
            int64_t lhs = popStack().getInt64();
            int64_t rhs = popStack().getInt64();
            if (lhs < rhs) 
            {
                _stack.push((int64_t) -1);
            } 
            if (lhs > rhs)
            {
                _stack.push((int64_t) 1);
            }
            if (lhs == rhs) 
            {
                _stack.push((int64_t) 0);
            } 
            break;
        };
        case BC_DCMP: 
        {
            double lhs = popStack().getDouble();
            double rhs = popStack().getDouble();
            if (lhs < rhs) 
            {
                _stack.push((int64_t) -1);
            } 
            if (lhs > rhs)
            {
                _stack.push((int64_t) 1);
            }
            if (lhs == rhs) 
            {
                _stack.push((int64_t) 0);
            } 
            break;
        };
        case BC_IFICMPNE:
        {
            int64_t lhs = popStack().getInt64();
            int64_t rhs = popStack().getInt64();
            _context->jumpIf(lhs != rhs);
            break;
        }
        case BC_IFICMPE:
        {
            int64_t lhs = popStack().getInt64();
            int64_t rhs = popStack().getInt64();
            _context->jumpIf(lhs == rhs);
            break;
        }
        case BC_IFICMPG:
        {
            int64_t lhs = popStack().getInt64();
            int64_t rhs = popStack().getInt64();
            _context->jumpIf(lhs > rhs);
            break;
        }
        case BC_IFICMPGE:
        {
            int64_t lhs = popStack().getInt64();
            int64_t rhs = popStack().getInt64();
            _context->jumpIf(lhs >= rhs);
            break;
        }
        case BC_IFICMPL:
        {
            int64_t lhs = popStack().getInt64();
            int64_t rhs = popStack().getInt64();
            _context->jumpIf(lhs < rhs);
            break;
        }
        case BC_IFICMPLE:
        {
            int64_t lhs = popStack().getInt64();
            int64_t rhs = popStack().getInt64();
            _context->jumpIf(lhs <= rhs);
            break;
        }
        case BC_JA:
            _context->jumpIf(true);
            break;
        case BC_STOP:
            break;
        default:
            return false;
    }
    
    return true;
}

bool CodeInterpreter::tryOther(Instruction Instruction) 
{
    switch (Instruction)
    {
        case BC_CALL: 
        {
            uint16_t funcId = _context->getUInt16();
            BytecodeFunction* function = (BytecodeFunction*) functionById(funcId);
            _context = new InterpreterContext(function, _context);
            break;
        }
        case BC_CALLNATIVE: 
        {
            _context->getUInt16();
            throw InterpreterException("NATIVE CALL");
        }
        case BC_RETURN: 
        {
            InterpreterContext* context = _context;
            _context = _context->getParentContext();
            delete context;
            break;
        }
        case BC_SWAP:
        {
            StackElement top = popStack();
            std::swap(top, _stack.top());
            // new top
            _stack.push(top);
            break;
        }
        case BC_POP:
            _stack.pop();
            break;
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

