#ifndef CONTEXT_H
#define CONTEXT_H

#include "mathvm.h"
#include "my_utils.h"

namespace mathvm {

class Context {
private:
    map<string, uint16_t> _varnameIdMap;
    uint16_t _id;
    uint16_t _parentId;
    bool _isTopmost;

    uint16_t _lastVarId;

public:
    Context(uint16_t id)
        : _id(id)
        , _parentId(0)
        , _isTopmost(true)
        , _lastVarId(0)
    {}

    Context(uint16_t id, uint16_t parentId)
        : _id(id)
        , _parentId(parentId)
        , _isTopmost(false)
        , _lastVarId(0)
    {}

    Context(Context const& other)
        : _varnameIdMap(other._varnameIdMap)
        , _id(other._id)
        , _parentId(other._parentId)
        , _isTopmost(other._isTopmost)
        , _lastVarId(other._lastVarId)
    {}

    Context& operator=(Context const& other) {
        if(this != &other) {
            Context(other).swap(*this);
        }
        return *this;
    }

    uint16_t id() { return _id; }
    uint16_t parentId() { return _parentId; }
    bool isTopmost() { return _isTopmost; }
    size_t varsNumber() const { return _varnameIdMap.size(); }

    bool hasVar(string const& name) {
        auto it = _varnameIdMap.find(name);
        return it != _varnameIdMap.end();
    }

    uint16_t addVar(string const& name) {
        if(_lastVarId >= UINT16_MAX) {
            throw ExceptionWithMsg(
                        "Too much variables created. This VM doesn't support more than 65535 variables per context");
        }
        if(hasVar(name)) {
            throw ExceptionWithMsg("attempt to add var twice, var name: " + name);
        }
        _varnameIdMap.insert(make_pair(name, _lastVarId));
        return _lastVarId ++;
    }

    uint16_t getVarId(string const& name) {
        auto it = _varnameIdMap.find(name);
        if(it == _varnameIdMap.end()) {
            throw ExceptionWithMsg("no var with name " + name);
        }
        return it->second;
    }

    void swap(Context &other) {
        std::swap(_varnameIdMap, other._varnameIdMap);
        std::swap(_id, other._id);
        std::swap(_parentId, other._parentId);
        std::swap(_isTopmost, other._isTopmost);
        std::swap(_lastVarId, other._lastVarId);
    }
};

}
#endif // CONTEXT_H
