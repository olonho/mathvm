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
    switch (instruction)
    {
        default:
            throw new InterpreterException("Invalid instruction for interpreter");
    }
}



