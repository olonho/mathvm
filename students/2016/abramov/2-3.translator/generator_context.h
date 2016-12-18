#pragma once

#include <map>
#include <cstdint>

#include "ast.h"
#include "mathvm.h"
#include "generator_exception.h"

namespace mathvm
{
    class Context {
    public:
        struct VarInfo;

    public:
        Context(BytecodeFunction* bytecodeFunc, Scope* scope, Context* parentContext = nullptr);
        Context(const Context& other);
        virtual ~Context();

        // Getters
        BytecodeFunction* getBytecodeFunction();
        Context* getParentContext();
        uint16_t getContextId() const;
        uint16_t getVarsSize() const;
        Context::VarInfo getVariable(const std::string& name) const;
        
        // 
        void addVariable(const AstVar* variable);

    private:
        BytecodeFunction* _byteCodeFunc;
        uint16_t _contextId;
        Context* _parentContext;
        std::map<std::string, uint16_t> _vars;
    };

    struct Context::VarInfo
    {
        VarInfo(uint16_t var_id, uint16_t context_id)
            : _varId(var_id)
            , _contextId(context_id)
        {}

        const uint16_t _varId;
        const uint16_t _contextId;
    };
}



