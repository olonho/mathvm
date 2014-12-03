#ifndef BLOCKSCOPE_HPP
#define BLOCKSCOPE_HPP

#include <string>
#include <vector>
#include <map>
#include <utility>

using std::string;
using std::vector;
using std::map;
using std::pair;

#include "ast.h"
#include "mathvm.h"

using mathvm::Signature;
using mathvm::AstVar;
using mathvm::Var;
using mathvm::VarType;

class BlockScope {

public:
    BlockScope(BlockScope *parent)
        : _functionId(parent->functionId()), _offset(parent->_offset + parent->_vars.size()), _parent(parent) {
    }

    BlockScope(uint16_t functionId, BlockScope *parent = 0, const Signature *signature = 0)
        : _functionId(functionId), _offset(0), _parent(parent) {
        if (!signature) { return; }
        for (auto it = signature->begin() + 1; it != signature->end(); ++it) {
            defineVar(it->first, it->second);
        }
    }

    ~BlockScope() {}

    uint16_t defineVar(const AstVar *astVar);
    uint16_t defineVar(const VarType & type, const string & name);
    pair<uint16_t, uint16_t> resolveVar(const string & name) const;

    BlockScope *parent() const { return _parent; }
    uint16_t functionId() const { return _functionId;}
    uint16_t offset() const { return _offset; }

private:
    const uint16_t _functionId;
    const uint16_t _offset;
    BlockScope *const _parent;
    vector<Var> _vars;
    map<string, uint16_t> _nameToId;

};

#endif // BLOCKSCOPE_HPP
