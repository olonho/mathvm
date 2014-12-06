#include "blockscope.hpp"

using std::make_pair;

uint16_t BlockScope::defineVar(const AstVar *astVar) {
    return defineVar(astVar->type(), astVar->name());
}

uint16_t BlockScope::defineVar(const VarType & type, const string & name) {
    if (_nameToId.find(name) != _nameToId.end()) {
        return _nameToId[name];
    }
    Var v(type, name);
    uint16_t varId = _vars.size();
    _nameToId[name] = varId;
    _vars.push_back(v);
    return varId;
}

pair<uint16_t, uint16_t> BlockScope::resolveVar(const string & name) const {
    const BlockScope *current = this;
    while (current != 0) {
        auto it = current->_nameToId.find(name);
        if (it != current->_nameToId.end()) {
            return make_pair(current->function()->id(), current->offset() + it->second);
        } else {
            current = current->parent();
        }
    }
    throw "Variable not found";
}
