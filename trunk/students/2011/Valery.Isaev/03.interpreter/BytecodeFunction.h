#ifndef _BYTECODE_FUNCTION_H_
#define _BYTECODE_FUNCTION_H_

#include "mathvm.h"
#include "ast.h"

struct Bytecode: mathvm::Bytecode {
    uint8_t* bytecode() { return _data.data(); }
};

class BytecodeFunction: public mathvm::TranslatedFunction {
    Bytecode _bytecode;
    std::vector<const mathvm::AstVar*> vars;
    std::vector<mathvm::AstNode*> nodes;
public:
    BytecodeFunction(mathvm::AstFunction* f): mathvm::TranslatedFunction(f) {}
    template<class T> void addFreeVar(T* t) {
        for (uint32_t i = 0; i < vars.size(); ++i) {
            if (vars[i]->name() == t->var()->name()) {
                return;
            }
        }
        nodes.push_back(t);
        vars.push_back(t->var());
    }
    uint32_t freeVars() const { return vars.size(); }
    const mathvm::AstVar* varAt(uint32_t i) const { return vars[i]; }
    mathvm::AstNode* nodeAt(uint32_t i) { return nodes[i]; }
    Bytecode* bytecode() { return &_bytecode; }
    void disassemble(std::ostream& out) const { _bytecode.dump(out); }
};

#endif
