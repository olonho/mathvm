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
using mathvm::BytecodeFunction;

class BlockScope {

public:
    BlockScope(BlockScope *parent)
        : _parent(parent),
          _function(parent->function()),
          _offset(parent->_offset + parent->_vars.size()),
          _locals(0) {
    }

    BlockScope(BytecodeFunction *function, BlockScope *parent = 0, const Signature *signature = 0)
        : _parent(parent),
          _function(function),
          _offset(0),
          _locals(0) {
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
    BytecodeFunction *function() const { return _function;}
    uint16_t offset() const { return _offset; }
    uint16_t size() const { return _vars.size(); }
    uint16_t childLocals() const { return _locals; }
    void setChildLocals(uint16_t locals) { _locals = locals > _locals ? locals : _locals; }

private:
    BlockScope *const _parent;
    BytecodeFunction *const _function;
    const uint16_t _offset;
    vector<Var> _vars;
    map<string, uint16_t> _nameToId;
    uint16_t _locals;                   // number of locals in child blocks
};

#endif // BLOCKSCOPE_HPP
