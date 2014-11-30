#ifndef VMSCOPEVARS_H
#define VMSCOPEVARS_H

#include <vector>
#include <map>
#include "mathvm.h"

class vmScopeVars
{
    typedef int16_t idType;
    typedef std::map<idType, std::pair<mathvm::Var*,bool> > scopeMapType;

public:
    vmScopeVars();
    void addVar(idType id, mathvm::Var* var);
    mathvm::Var* getVar(idType id);
    void newScope();
    void back();

private:
    std::vector<scopeMapType> _scope;
};

#endif
