#pragma once

#include <map>
#include <cstdint>

#include "mathvm.h"
#include "generator_exception.h"

namespace mathvm
{
    class Context {
    public:
        struct VarInfo;

    public:
        Context();
        Context(const Context& orig);
        virtual ~Context();

        BytecodeFunction* getBytecodeFunction();
        Context* getParentContext();
        uint16_t getContextId() const;
        Context::VarInfo getVariable(const std::string& name) const;

    private:
        Context* _parentContext;
        BytecodeFunction* _byteCodeFunc;
        std::map<std::string, uint16_t> _vars;
        uint16_t _contextId;
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



