#pragma once

#include "mathvm.h"
#include "my_translator.h"

using namespace mathvm;

class BytecodeScope {
public:
    BytecodeScope(Code* targetCode);
    BytecodeScope(BytecodeScope* parent, idType id);
    ~BytecodeScope();
    idType registerFunction(BytecodeFunction* bytecodeFunction);
    idType registerVariable(VarType varType, std::string const & varName);
    idType getStringConstant(std::string const& str);
    BytecodeFunction* getFunction(std::string const& functionName);
    idPair getVariableAndScope(std::string const& variableName);
    Var* getVariable(idPair variableId);
    BytecodeScope* createChild();
    const idType id;
    uint16_t varsCount();
private:
    bool isRoot();
    Code* _targetCode;
    vector<BytecodeScope*>* scopesStack;
    vector<BytecodeScope*> childScopes;
    BytecodeScope* parentScope;
    std::map<std::string, idType> functionMap;
    std::map<std::string, idType> variableMap;
    std::vector<Var*> variables;
};