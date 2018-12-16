#ifndef MATHVM_CONTEXT_H
#define MATHVM_CONTEXT_H

#include <map>
#include <string>
#include "mathvm.h"
#include "ast.h"

namespace mathvm {

    class VisitorCtx {

    public:
        const uint16_t address;
        map<string, uint16_t> variables;
        VisitorCtx *parent;

        VisitorCtx(uint16_t addr, VisitorCtx *parent) : address(addr), parent(parent) {}

        void addVariable(const string &name) {
            variables.insert(make_pair(name, variables.size()));
        }
    };
}

#endif
