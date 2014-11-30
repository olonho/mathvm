#include "abstractvarcontext.h"
using std::vector;
using std::map;
using std::string;
using mathvm::VarType;

AbstractVarContext::AbstractVarContext():
    g_var_id(0),
    vars(1)
{
}

int16_t AbstractVarContext::presentVar(const std::string & name, const VarType type)
{
    vars.back()[name] = g_var_id;
    var_types[g_var_id] = type;
    return g_var_id++;
}

int16_t AbstractVarContext::getVar(const std::string&  name){
    for(int i = vars.size()-1 ; i > -1; --i){
        if (vars[i].empty()) continue;
        if (vars[i].find(name) != vars[i].end()){
            return vars[i][name];
        }
    }
    return -1;
}


int16_t AbstractVarContext::getVar(const std::string&  name, int fromId, int toId = -1){
    if (toId < 0) toId = vars.size();
    if (fromId < 0) fromId = 0;
    for(int i = fromId; i < toId && i < (int)vars.size(); ++i){
        if (vars[i].empty()) continue;
        if (vars[i].find(name) != vars[i].end()){
                return vars[i][name];
        }
    }
    return -1;
}


VarType AbstractVarContext::getVarType(int16_t id){
    if (var_types.find(id) != var_types.end()){
        return var_types[id];
    }
    return mathvm::VT_INVALID;
}

int16_t  AbstractVarContext::freshContext(){
    vars.push_back(map<string, int16_t>());
    return vars.size()-1;
}
