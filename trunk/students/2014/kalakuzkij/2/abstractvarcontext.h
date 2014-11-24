#ifndef ABSTRACTVARCONTEXT_H
#define ABSTRACTVARCONTEXT_H
#include <map>
#include <stdint.h>
#include <string>
#include <vector>
#include "mathvm.h"

class AbstractVarContext
{
public:
    AbstractVarContext();
    int16_t presentVar(const std::string& name, const mathvm::VarType type);
    int16_t getVar(const std::string & name);
    int16_t getVar(const std::string & name, int from_scope_id, int to_scope_id);
    mathvm::VarType getVarType(int16_t id);

    int16_t freshContext();

    class VarIterator{
        AbstractVarContext* parent;
        int from;
        int to;
        std::map<std::string, int16_t>::iterator cur_var;
    public:
        VarIterator(AbstractVarContext* avc, int from_scope, int to_scope){
            parent = avc;
            from = from_scope;
            to = to_scope;
            if (to > (int)avc->vars.size()) to = avc->vars.size();
            assert(from < (int)avc->vars.size() && from < to);
            cur_var = avc->vars[from].begin();
        }
        bool hasNext(){
            if (from >= to ) return false;
            if (from +1 == to && cur_var == parent->vars[from].end()) return false;
            return true;
        }
        void next(int &id, std::string& name, mathvm::VarType& vt){
            while (cur_var == parent->vars[from].end()){
                ++from;
                cur_var = parent->vars[from].begin();
            }
            id = cur_var->second;
            name = cur_var->first;
            vt = parent->var_types[id];
        }
    };

private:
    int16_t g_var_id;
    std::vector<std::map<std::string, int16_t> > vars;
    std::map<int16_t, mathvm::VarType> var_types;
};

#endif // ABSTRACTVARCONTEXT_H
