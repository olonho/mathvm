#include "generator_context.h"
#include "ast.h"

using namespace mathvm;

Context::Context(BytecodeFunction* bytecodeFunc, Scope* scope, Context* parentContext) 
    : _byteCodeFunc(bytecodeFunc)
    , _parentContext(parentContext)
{
    _contextId = (parentContext == nullptr) ? 0 : parentContext->_contextId + 1;
    for (Scope::VarIterator varIterator = Scope::VarIterator(scope); varIterator.hasNext();) 
    {
        addVariable(varIterator.next());
    }
}

Context::Context(const Context& other) 
    : _byteCodeFunc(other._byteCodeFunc)
    , _contextId(other._contextId)
    , _parentContext(other._parentContext)
{}

Context::~Context() 
{}

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

uint16_t Context::getVarsSize() const 
{
    return _vars.size();
}


void Context::addVariable(const AstVar* variable) 
{
    _vars[variable->name()] = _vars.size();
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










