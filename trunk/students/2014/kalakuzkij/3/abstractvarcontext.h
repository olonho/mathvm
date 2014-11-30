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
        int scope;
        int to;
        std::map<std::string, int16_t>::iterator cur_var;
    public:
        VarIterator(AbstractVarContext* avc, unsigned int scope){
            parent = avc;
            this->scope = scope;
            assert(scope < parent->vars.size());
            cur_var = parent->vars[scope].begin();
        }
        bool hasNext(){
            bool a = cur_var != parent->vars[scope].end();
            return a;
        }
        void next(int &id, std::string& name, mathvm::VarType& vt){
            id = cur_var->second;
            name = cur_var->first;
            vt = parent->var_types[id];
            ++cur_var;
        }
    };

private:
    int16_t g_var_id;
    std::vector<std::map<std::string, int16_t> > vars;
    std::map<int16_t, mathvm::VarType> var_types;
};

#endif // ABSTRACTVARCONTEXT_H
