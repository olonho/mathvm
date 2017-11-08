#ifndef CONTEXT_H
#define CONTEXT_H

#include "mathvm.h"
#include <unordered_map>

namespace mathvm {
    struct Context {
        Context(BytecodeFunction* func, Context* parent = nullptr);
        BytecodeFunction* function();
        std::pair<uint16_t, uint16_t> locAndCtxId(const std::string& name);
        void addVar(const std::string& name);
        uint16_t id() const;
        uint16_t localsCount() const;
        Context* parent();

    private:
        BytecodeFunction* _function;
        Context* _parent;
        uint16_t _curId;
        std::unordered_map<std::string, uint16_t> _varName2Id;
    };
}
#endif //CONTEXT_H
