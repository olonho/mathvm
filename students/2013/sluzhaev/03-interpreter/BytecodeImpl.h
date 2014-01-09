#pragma once

#include <vector>
#include <stack>

#include "mathvm.h"
#include "ast.h"

namespace mathvm {


struct InterpretationError {
    InterpretationError(std::string message) : message(message) {}
    
    std::string getMessage() const {
        return message;
    }
private:
    std::string message;
};

struct BytecodeImpl : Code {
    struct FunctionScope {
        FunctionScope(BytecodeFunction* function, FunctionScope* parent = 0) : function(function), parent(parent), ip(0) {
            for (size_t i = function->localsNumber() + function->parametersNumber(); i != 0; --i) {
                variables.push_back(Var(VT_INT, ""));
            }
        }

        BytecodeFunction* function;
        FunctionScope* parent;
        uint32_t ip;
        std::vector<Var> variables;
    };

    Status* execute(vector<Var*>& vars);
    Status* execute();

private:
    Var pop();
    void loadVariable(uint16_t id);
    void loadVariable(uint16_t context_id, uint16_t id);
    void storeVariable(uint16_t id);
    void storeVariable(uint16_t context_id, uint16_t id);
    void processDoubleBinaryOperation(TokenKind op);
    void processIntegerBinaryOperation(TokenKind op);
    void exec_dload(double val);
    void exec_iload(int64_t val);
    void exec_sload(uint16_t id);

    template<class T>
    T getValue(uint16_t shift) {
        T value = currentScope->function->bytecode()->getTyped<T>(currentScope->ip);
        currentScope->ip += shift;
        return value;
    }

    FunctionScope* currentScope;
    std::stack<Var> variables;
};

}
