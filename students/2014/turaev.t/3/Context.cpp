#include "Context.hpp"
#include "Errors.hpp"

namespace mathvm {
    uint16_t Context::introduceFunction(BytecodeFunction *function) {
        if (code->functionByName(function->name()) != 0) {
            throw TranslationError("Can't add function with name " + function->name() +
                    ". Function with same name exists in context");
        }
        uint16_t id = code->addFunction(function);
        function->setScopeId(id);
        return id;
    }

    Context *Context::addChildContext() {
        Context *context = new Context(this, (uint16_t) childContexts->size());
        childContexts->push_back(context);
        return context;
    }

    BytecodeFunction *Context::getFunction(string const &name) {
        BytecodeFunction *function = (BytecodeFunction *) code->functionByName(name);
        if (function == NULL) {
            throw TranslationError("Can't find function " + name);
        }
        return function;
    }

    uint16_t Context::introduceVariable(VarType type, string const &name) {
        if (variablesMap.find(name) != variablesMap.end())
            throw TranslationError("Can't add variable with name '" + name + "': variable with same name exists in context");
        uint16_t id = (uint16_t) variablesMap.size();
        variablesMap[name] = id;
        variablesList.push_back(new Var(type, name));
        return id;
    }

    VariableInContextDescriptor Context::getVariableDescriptor(string const &name) {
        VariablesMap::const_iterator it = variablesMap.find(name);
        if (it == variablesMap.end()) {
            if (parentContext == NULL) {
                throw TranslationError("Variable '" + name + "' not found");
            }
            return parentContext->getVariableDescriptor(name);
        }
        return make_pair(contextID, it->second);
    }

    Var *Context::getVariableByID(VariableInContextDescriptor descriptor) {
        assert(descriptor.first < childContexts->size());
        Context *ctx = childContexts->at(descriptor.first);
        assert(descriptor.second < ctx->variablesMap.size());
        return ctx->variablesList.at(descriptor.second);

    }
}