#ifndef _LABEL_H_
#define _LABEL_H_

#include "ast.h"
#include "mathvm.h"
#include "BytecodeFunction.h"
#include <set>

struct FunInfo {
    std::set<const mathvm::AstVar*> freeVars;
    std::set<std::pair<FunInfo*, mathvm::Scope*> > callees;
    BytecodeFunction* bcfun;
};

class Label {
    mathvm::Label label;
    mathvm::Bytecode* code;
public:
    Label(mathvm::Bytecode* c): label(c), code(c) {}
    ~Label() {
        if (!label.isBound()) {
            code->bind(label);
        }
    }
    mathvm::Label& operator()() { return label; }
};


#endif
