#ifndef VARIABLE_CONTEXT_H__
#define VARIABLE_CONTEXT_H__

#include <map>
#include <memory>

using namespace std;

class variable_context;

typedef map<string, unsigned int> map_variable;
typedef pair<unsigned int, unsigned int> id_variable;
typedef shared_ptr<variable_context> prt_var;

class variable_context {
public:
    variable_context(unsigned int id = 0) : _id(id) {}

    id_variable find_id_variable(const string &name) {
        auto resultIterator = _mapVar.find(name);
        if (resultIterator != _mapVar.end()) {
            return id_variable(_id, resultIterator->second);
        } else {
            return id_variable();
        }
    }

    bool variableExists(const string &name) {
        auto resultIterator = _mapVar.find(name);
        return resultIterator != _mapVar.end();
    }

    unsigned int newVar(const string name) {
        _overflowed = _mapVar.size() == UINT16_MAX;
        auto newId = _mapVar.size();
        return _mapVar[name] = newId;
    }

    bool hasOverflowed() {
        return _overflowed;
    }

    unsigned int contextId() {
        return _id;
    }

    unsigned int localsNumber() {
        return _mapVar.size();
    }

private:
    unsigned int _id;
    bool _overflowed = false;
    prt_var _parent;
    map_variable _mapVar;
};

#endif
