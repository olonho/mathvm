#include <limits>
#include "translator_context.h"

#include <sstream>

using namespace mathvm;
using std::vector;
using std::map;
using std::pair;
using std::string;


void TranslatorContext::pushScope(Scope *scope, map<string, std::pair<BytecodeFunction*, bool>> functions) {
    assert(scope->parent() == _cur_scope || _cur_scope == nullptr);
    if (_context_ids.empty()) {
        _context_ids.push(0);
    }
    _context_ids.push(_context_ids.top());
    _f_bytecodes.push(_f_bytecodes.top());
    setCurScope(scope);
    addVarsFromScope(scope);

    Scope::FunctionIterator funIt(scope);
    while (funIt.hasNext()) {
        auto fun = funIt.next();
        auto p = functions[fun->name()];
        auto res = _funs.insert(std::make_pair(fun->name(), p));
        if (!res.second) {
            throw TranslatorError("Function [" + fun->name() + "] is already in scope");
        }
    }
}

void TranslatorContext::pushFunctionScope(Scope *scope, BytecodeFunction *f) {
    assert(scope->parent() == _cur_scope || _cur_scope == nullptr);

    if (_context_ids.empty()) {
        _context_ids.push(0);
    } else {
        _context_ids.push(_context_ids.top() + (uint16_t) 1); // inc context id
    }
    _f_bytecodes.push(f);
    _locals_counts.push(0);
    setCurScope(scope);
    addVarsFromScope(scope);
    return_pos = -1;
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
    if (_context_ids.empty() || _context_ids.top() != prev) {
        _locals_counts.pop();
    }
    _f_bytecodes.pop();

}

VarData TranslatorContext::getVarByName(const std::string &var_name) {
    auto it = _vars.find(var_name);
    if (it == _vars.end() || it->second.empty()) {
        throw TranslatorError("No such var [" + var_name + "]");
    }
    return it->second.back();
}

void TranslatorContext::addVarsFromScope(Scope *scope) {
    Scope::VarIterator varIt(scope);
    if (scope->variablesCount() > std::numeric_limits<uint16_t>::max()) {
        std::stringstream ss;
        ss << "Too much vars in one scope: " << scope->variablesCount() << ".";
        throw TranslatorError(ss.str());
    }
    while (varIt.hasNext()) {
        auto var = varIt.next();
        VarData var_data = {var->type(), currentContextId(), (uint16_t) _locals_counts.top(), var->name()};
        _locals_counts.top() += 1;
        auto it = _vars.find(var->name());
        if (it == _vars.end()) {
            _vars.insert(it, std::make_pair(var->name(), std::vector<VarData>(1, var_data)));
        } else {
            it->second.push_back(var_data);
            // TODO: printing it is not a very good idea
            std::cerr << "Warning: variable " + var->name() + " shadows variable from outer scope" << std::endl;
        }
    }
}

void TranslatorContext::removeVarsFromCurScope() {
    Scope::VarIterator varIt(_cur_scope);
    while (varIt.hasNext()) {
        auto var = varIt.next();
        auto it = _vars.find(var->name());
        assert(it != _vars.end());
        auto& var_vec = it->second;
        assert(var_vec.back().context == currentContextId());
        var_vec.pop_back();
        if (var_vec.empty()) {
            _vars.erase(it);
        }
    }
}

std::pair<BytecodeFunction*, bool> TranslatorContext::getFunByName(const std::string &fun_name) {
    auto it = _funs.find(fun_name);
    if (it == _funs.end()) {
        throw TranslatorError("No such fun [" + fun_name + "]");
    }
    return it->second;
}
