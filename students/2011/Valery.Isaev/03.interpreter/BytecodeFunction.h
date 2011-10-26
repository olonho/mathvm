#ifndef _BYTECODE_FUNCTION_H_
#define _BYTECODE_FUNCTION_H_

#include <set>

#include "mathvm.h"
#include "ast.h"

struct Bytecode: mathvm::Bytecode {
    uint8_t* bytecode() { return _data.data(); }
};

class BytecodeFunction: public mathvm::TranslatedFunction {
    struct Pair {
        std::string var;
        const mathvm::AstNode* node;
        bool operator<(const Pair& p) const {
            return var < p.var;
        }
        Pair(const std::string& _var, const mathvm::AstNode* _node)
            : var(_var), node(_node) {}
    };
    Bytecode _bytecode;
    std::set<Pair> _vars;
public:
    typedef std::set<Pair>::iterator iterator;
    BytecodeFunction(mathvm::AstFunction* f): mathvm::TranslatedFunction(f) {}
    template<class T> void addFreeVar(const T* t) {
        _vars.insert(Pair(t->var()->name(), t));
    }
    const std::set<Pair>& vars() const { return _vars; }
    Bytecode* bytecode() { return &_bytecode; }
    void disassemble(std::ostream& out) const { _bytecode.dump(out); }
};

#endif
