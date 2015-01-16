#include "my_translator.h"
#include "bytecodeScope.h"
#include "translatorVisitor.h"

idType BytecodeScope::registerFunction(BytecodeFunction* bytecodeFunction) {
    idType  functionId = _targetCode->addFunction(bytecodeFunction);
    if (functionId == INVALID_ID || functionMap.find(bytecodeFunction->name()) != functionMap.end()) return INVALID_ID;
    bytecodeFunction->setScopeId(id);
    functionMap.insert(std::map<std::string, idType>::value_type(bytecodeFunction->name(), functionId));
    return functionId;
}

idType BytecodeScope::registerVariable(VarType varType, std::string const &varName) {
//    std::cout << "registered variable " << varName << " of type " << varType << "\n"; //LOG
    if (variableMap.find(varName) != variableMap.end() || variables.size() == INVALID_ID) return INVALID_ID;
    idType variableId = variables.size();
    variables.push_back(new Var(varType, varName));
    variableMap.insert(std::map<std::string, idType>::value_type(varName, variableId));
    return variableId;
}

Var* BytecodeScope::getVariable(idPair variableId) {
    idType scopeId = variableId.first;
    idType varId = variableId.second;
    if (scopeId >= scopesStack->size()) return NULL;
    BytecodeScope* varScope = (*scopesStack)[scopeId];
    if (varId >= varScope->variables.size()) return NULL;
    return varScope->variables[varId];
}

idType BytecodeScope::getStringConstant(std::string const& str) {
   return _targetCode->makeStringConstant(str);
}

BytecodeScope::BytecodeScope(BytecodeScope* parent, idType _id): id(_id), _targetCode(parent->_targetCode), scopesStack(parent->scopesStack), parentScope(parent){}

BytecodeScope* BytecodeScope::createChild() {
    idType scopeId = scopesStack->size();
    if (scopeId == INVALID_ID) return NULL;
    BytecodeScope* scope = new BytecodeScope(this, scopeId);
    scopesStack->push_back(scope);
    return scope;
}

uint16_t BytecodeScope::varsCount() {
    return variables.size();
}

idPair BytecodeScope::getVariableAndScope(std::string const& variableName) {
    std::map<std::string, idType>::iterator varIt = variableMap.find(variableName);
    if (varIt == variableMap.end()) {
        return parentScope != NULL ? parentScope->getVariableAndScope(variableName) : std::make_pair(INVALID_ID, INVALID_ID);
    }
    return std::make_pair(id, varIt->second);
}

BytecodeFunction* BytecodeScope::getFunction(std::string const& functionName) {
    map<std::string, idType>::iterator res = functionMap.find(functionName);
    if (res == functionMap.end()) {
        return (BytecodeFunction *) (parentScope != NULL ? parentScope->getFunction(functionName) : NULL);
    }
    return dynamic_cast<BytecodeFunction *>(_targetCode->functionById(res->second));

}

BytecodeScope::BytecodeScope(Code* targetCode): id(0), _targetCode(targetCode), scopesStack(new vector<BytecodeScope*>()),
    parentScope(NULL) {
        scopesStack->push_back(this);
}

bool BytecodeScope::isRoot() {
    return parentScope == NULL;
}

BytecodeScope::~BytecodeScope() {
    if (isRoot()) {
        delete scopesStack;
    }
}

Status* BytecodeTranslatorImpl::translateBytecode(const string& program,
        InterpreterCodeImpl* *code) {
    return translate(program, (Code**) code);
}

Status* BytecodeTranslatorImpl::translate(const string &program, Code **code) {
    Parser parser;
    Status* parseResult = parser.parseProgram(program);
    if (!parseResult) {
        //TODO: report 'null' status
    } else if (parseResult->isError()) {
        //TODO: report parser error
    } else {
        //parsing succeeded, now can translate
        BytecodeImpl *resultCode = new BytecodeImpl;
        BytecodeScope rootScope(resultCode);
        BytecodeFunction* rootFunction = new BytecodeFunction(parser.top());
        rootScope.registerFunction(rootFunction);

        TranslatorVisitor visitor(&rootScope, NULL);

        try {
            visitor.visitFunctionNode(parser.top()->node());

            *code = resultCode;
        } catch (std::logic_error const& e) {
            return Status::Error(e.what());
        }
        return Status::Ok();
    }
    return Status::Ok();
}
