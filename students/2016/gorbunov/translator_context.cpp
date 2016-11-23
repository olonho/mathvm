#include "translator_context.h"

using namespace mathvm;
using std::vector;
using std::map;
using std::pair;
using std::string;


void TranslatorContext::pushScope(Scope *scope, map<string, BytecodeFunction*> functions) {
    assert(scope->parent() == _cur_scope || _cur_scope == nullptr);
    _context_ids.push(_context_ids.top());
    _f_bytecodes.push(_f_bytecodes.top());
    setCurScope(scope);
    addVarsFromScope(scope);

    Scope::FunctionIterator funIt(scope);
    while (funIt.hasNext()) {
        auto fun = funIt.next();
        auto bytecodeFun = functions[fun->name()];
        auto res = _funs.insert(std::make_pair(fun->name(), bytecodeFun));
        if (!res.second) {
            throw TranslatorError("Function [" + fun->name() + "] is already in scope");
        }
    }
}

void TranslatorContext::pushFunctionScope(Scope *scope, BytecodeFunction *f) {
    assert(scope->parent() == _cur_scope || _cur_scope == nullptr);
    _context_ids.push(_context_ids.top() + (uint16_t) 1); // inc context id
    _f_bytecodes.push(f);
    _locals_counts.push(0);
    setCurScope(scope);
    addVarsFromScope(scope);
    returnPos = -1;
}


void TranslatorContext::popScope() {
    removeVarsFromCurScope();
    // deleting functions from this scope
    Scope::FunctionIterator funIt(_cur_scope);
    while (funIt.hasNext()) {
        auto fun = funIt.next();
        _funs.erase(fun->name());
    }

    _cur_scope = _cur_scope->parent();
    auto prev = _context_ids.top();
    _context_ids.pop();
    if (_context_ids.top() != prev) {
        _locals_counts.pop();
    }
    _f_bytecodes.pop();

}

VarData TranslatorContext::getVarByName(const std::string &var_name) {
    auto it = _vars.find(var_name);
    if (it == _vars.end()) {
        throw TranslatorError("No such var [" + var_name + "]");
    }
    return it->second;
}

void TranslatorContext::addVarsFromScope(Scope *scope) {
    Scope::VarIterator varIt(scope);
    while (varIt.hasNext()) {
        auto var = varIt.next();
        VarData var_data = {var->type(), currentContextId(), (uint16_t) _locals_counts.top(), var->name()};
        _locals_counts.top() += 1;
        auto res = _vars.insert(std::make_pair(var->name(), var_data));
        if (!res.second) {
            throw TranslatorError("Variable [" + var->name() + "] already in scope stack");
        }
    }
}

void TranslatorContext::removeVarsFromCurScope() {
    Scope::VarIterator varIt(_cur_scope);
    while (varIt.hasNext()) {
        auto var = varIt.next();
        _vars.erase(var->name());
    }
}

BytecodeFunction* TranslatorContext::getFunByName(const std::string &fun_name) {
    auto it = _funs.find(fun_name);
    if (it == _funs.end()) {
        throw TranslatorError("No such fun [" + fun_name + "]");
    }
    return it->second;
}
