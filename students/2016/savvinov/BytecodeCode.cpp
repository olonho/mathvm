//
// Created by dsavvinov on 11.11.16.
//

#include "BytecodeCode.h"
#include "Interpreter.h"

namespace mathvm {
Status * BytecodeCode::execute(vector<Var *> &vars) {
    Interpreter interpreter = Interpreter(this, vars);
    return interpreter.executeProgram();
}

int16_t BytecodeCode::saveScope(Scope *scope) {
    if (idByScope.find(scope) != idByScope.end()) {
        return idByScope[scope];
    }
    int16_t id = (int16_t) idByScope.size();
    idByScope[scope] = id;
    scopeById[id] = scope;
    return id;
}

Scope *BytecodeCode::getScope(int16_t id) {
    return scopeById[id];
}

}