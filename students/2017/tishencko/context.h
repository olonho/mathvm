#ifndef CONTEXT
#define CONTEXT

#include<map>
#include<string>
#include "mathvm.h"
#include "ast.h"

namespace mathvm {

using namespace std;

class Context {
    const uint16_t _addr;
    BytecodeFunction* _owner = nullptr;
public:
    map<string, uint16_t> variableMap;
    map<string, uint16_t> functionMap;
    map<uint16_t, Var> varMap;
    Context* parent;

    Context(uint16_t addr, Context* parent): _addr(addr), parent(parent) {
        variableMap = map<string, uint16_t>();
        functionMap = map<string, uint16_t>();
        varMap = map<uint16_t, Var>();
    }

//    Context(uint16_t addr, map<string, uint16_t> variables, map<string, uint16_t> funcs, map<uint16_t, Var> vars, ) {
//        _addr = addr;
//        parent = nullptr;
//        variableMap = variables;
//        functionMap = funcs;
//        varMap = vars;
//    }
    Context(const Context &rhs) = default;

    Context(uint16_t addr): _addr(addr) {
        parent = nullptr;
        variableMap = map<string, uint16_t>();
        functionMap = map<string, uint16_t>();
        varMap = map<uint16_t, Var>();
    }

    void owner(BytecodeFunction* fun) {
        _owner = fun;
    }

    BytecodeFunction* owner() {
        return _owner;
    }

    void addScope(Scope* scope, Code* code) {
        Scope::VarIterator varIt(scope);
        while(varIt.hasNext()) {
           AstVar * var = varIt.next();
           addVariable(var->name());
        }
        Scope::FunctionIterator funIt(scope);
        while(funIt.hasNext()) {
           AstFunction * fun = funIt.next();
           BytecodeFunction * bytecodeFun = new BytecodeFunction(fun);
           int16_t idx = code->addFunction(bytecodeFun);
           addFunction(fun->name(), idx);
        }
    }

    void addVariable(const string& name) {
        variableMap.insert(make_pair(name, variableMap.size()));
    }

    void addFunction(const string& name, uint16_t idx) {
        functionMap.insert(make_pair(name, idx));
    }

    bool includeFunc(const string& name) {
        return functionMap.find(name) != functionMap.end();
    }

    bool includeVar(const string& name) {
        return variableMap.find(name) != variableMap.end();
    }

    bool includeVar(uint16_t addr) {
        return varMap.find(addr) != varMap.end();
    }

    uint16_t addr() {
        return _addr;
    }

    uint16_t findVarAddr(const string& name) {
        return variableMap.at(name);
    }

    uint16_t findFunAddr(const string& name) {
        return functionMap.at(name);
    }

    void setVar(uint16_t id, Var var) {
        varMap.erase(id);
        varMap.insert(make_pair(id, var));
    }

    Var getVar(uint16_t id) {
        if (varMap.count(id) == 0) {
            Var var(VT_INT, "default");
            var.setIntValue(0);
            varMap.insert(make_pair(id, var));
        }
        return varMap.at(id);
    }

};

}

#endif // CONTEXT

