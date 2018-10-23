//
// Created by jetbrains on 18.10.18.
//

#ifndef VIRTUAL_MACHINES_SCOPE_H
#define VIRTUAL_MACHINES_SCOPE_H

#include <vector>
#include <map>
#include <mathvm.h>
#include <ast.h>

using namespace std;

namespace mathvm {
    class Context {
    private:
        typedef map<string, uint16_t> VariableMap;

        BytecodeFunction* _function;
        Context* _parentContext;
        uint32_t _bytecodePosition;
        uint16_t _scopeId;

        vector<Var*> _variables;
        VariableMap _variableById;
        VarType _typeOnStackTop;

    public:

        explicit Context(BytecodeFunction* function, Context *parentScope = nullptr, Scope *scope = nullptr);
        uint16_t addVar(Var* var);
        uint16_t addVar(AstVar* var);

        Var* getVarById(uint16_t index);
        uint16_t getVarId(string name);
        Var* getVar(string name);
        Context* getParentContext();

        uint32_t getBytecodePosition();

        uint16_t getContextId();

        Bytecode *getBytecode();

        BytecodeFunction *getFunction();

        VarType getTypeOnStackTop();

        void setTypeOnStackTop(VarType type);

        uint16_t getVarsCount();
    };
}

#endif //VIRTUAL_MACHINES_SCOPE_H
