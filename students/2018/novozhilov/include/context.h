//
// Created by jetbrains on 18.10.18.
//

#ifndef VIRTUAL_MACHINES_SCOPE_H
#define VIRTUAL_MACHINES_SCOPE_H

#include <map>
#include <vector>
#include <variant>
#include <mathvm.h>
#include <ast.h>

namespace mathvm {
    using StackValue = variant<int64_t , double , string>;

    class Context {
    public:

    private:
        typedef map<string, uint16_t> VariableMap;

        BytecodeFunction* _function;
        Context* _parentContext;

        vector<StackValue> _variables;
        VariableMap _variableById;

    public:
        explicit Context(BytecodeFunction* function, Context *parentScope = nullptr, Scope *scope = nullptr);
        uint16_t addVar(Var* var);
        uint16_t addVar(AstVar* var);
        uint16_t getVarId(string name);
        bool containsVariable(string name);
        uint16_t getContextId();
        Bytecode *getBytecode();
        uint16_t getVarsCount();



        // + +
        Context* getParentContext();
    };
}

#endif //VIRTUAL_MACHINES_SCOPE_H
