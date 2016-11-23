#include "context.h"

using namespace mathvm;

Context::Context() 
{
}

Context::Context(const Context& orig) 
{
}

Context::~Context() 
{
}

BytecodeFunction* Context::getBytecodeFunction()  
{
    return _byteCodeFunc;
}

Context* Context::getParentContext() 
{
    return _parentContext;
}

uint16_t Context::getContextId() const 
{
    return _contextId;
}

Context::VarInfo Context::getVariable(const std::string& name) const
{
    const auto it = _vars.find(name);
    if (it != _vars.end()) 
    {
        return Context::VarInfo(it->second, _contextId);
    }
    if (_parentContext == nullptr) 
    {
        throw GeneratorException("Couldn't find variable :" + name);
    }
    
    return _parentContext->getVariable(name);
}








