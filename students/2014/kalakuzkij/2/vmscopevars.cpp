#include "vmscopevars.h"
using mathvm::Var;

vmScopeVars::vmScopeVars()
{
    //_scope.reserve(100);
}

void vmScopeVars::addVar(idType id, mathvm::Var* var){
    _scope.back()[id] = std::make_pair(var, true);
}

Var* vmScopeVars::getVar(idType id){
    for (int i = _scope.size()-1; i >= 0; --i){
        if (_scope[i].find(id) != _scope[i].end()){
            for (unsigned int poor_idx = i+1; poor_idx < _scope.size(); ++poor_idx){
                _scope[poor_idx][id] = std::make_pair(_scope[i][id].first, false);
            }
            return _scope[i][id].first;
        }
    }
    return NULL;
}

void vmScopeVars::newScope(){
    _scope.push_back(scopeMapType());
}
void vmScopeVars::back(){
    //todo memory leak
    scopeMapType::iterator it = _scope.back().begin();
    for(; it != _scope.back().end(); ++it){
        if (it->second.second){
            delete it->second.first;
        }
    }
    _scope.pop_back();
}
