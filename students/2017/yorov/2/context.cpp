#include "context.h"
#include <limits>

namespace mathvm {
    Context::Context(BytecodeFunction* func, Context* parent)
        : _function(func)
        , _parent(parent)
        , _curId(0)
    {}

    BytecodeFunction* Context::function() {
        return _function;
    }

    void Context::addVar(const std::string& name) {
        if (_curId == std::numeric_limits<uint16_t>::max()) {
            throw std::logic_error("cannot create next id");
        }
        _varName2Id[name] = _curId;
        ++_curId;
    }

    std::pair<uint16_t, uint16_t> Context::locAndCtxId(const std::string& name) {
        if (_varName2Id.find(name) == _varName2Id.end()) {
            if (_parent != nullptr) {
                return _parent->locAndCtxId(name);
            } else {
                throw std::logic_error(std::string("cannot find variable ") + name);
            }
        }
        return std::make_pair(_varName2Id[name], _function->id());
    }

    uint16_t Context::id() const {
        return _function->id();
    }

    uint16_t Context::localsCount() const {
        return _varName2Id.size();
    }

    Context* Context::parent() {
        return _parent;
    }
}
