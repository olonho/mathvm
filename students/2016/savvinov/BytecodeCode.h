//
// Created by dsavvinov on 11.11.16.
//

#ifndef MATHVM_BYTECODECODE_H
#define MATHVM_BYTECODECODE_H

#include <unordered_map>
#include <ast.h>
#include "../../../include/mathvm.h"

namespace mathvm {
class BytecodeCode : public Code {
    std::unordered_map<int16_t, Scope *> scopeById;
    std::unordered_map<Scope *, int16_t> idByScope;
public:
    BytecodeCode() : Code() {
    }

    Status* execute(vector<Var*>& vars);

    int16_t saveScope(Scope * scope);
    Scope * getScope(int16_t id);
};

};
#endif //MATHVM_BYTECODECODE_H
