#ifndef INTERPRETER_CODE_IMPL
#define INTERPRETER_CODE_IMPL
#include "mathvm.h"
#include "vmstack.h"
#include <vector>
#include "abstractvarcontext.h"
#include "vmscopevars.h"
#include "asmjit/asmjit.h"
using namespace asmjit;
#include <map>
#include "ptrmap.h"

namespace mathvm{
class InterpreterCodeImpl:public Code{
private:
    JitRuntime runtime;

    vector<VarType> stack_types;
    vmStack _stack;
    AbstractVarContext a_vars;
    //vector<map<int16_t, Var*> >scope_vars;
    vmScopeVars scope_vars;
    bool running;

    void eval_func(BytecodeFunction* bf);
    Bytecode * init_function(BytecodeFunction* bf, vector<BytecodeFunction*>&  bf_stack);




 public:
    JitRuntime * getRuntime(){return &runtime;}

    virtual Status *execute(vector<Var*>& vars);
    ~InterpreterCodeImpl(){
        //todo clear this useless stuff
        int a = 0;
        a ++;
    }
    int16_t present_var(const string& name, const VarType type){
        return a_vars.presentVar(name, type);
    }
    int16_t get_var_by_name(const string& name){
        return a_vars.getVar(name);
    }
    int16_t get_var_by_name(const string& name, int scopeId){
        return a_vars.getVar(name, scopeId, scopeId+1);
    }
    VarType get_vartype_by_id(int16_t id){
        return a_vars.getVarType(id);
    }

    int16_t fresh_abs_var_context(){
        return a_vars.freshContext();
    }
//todo refacotr
        std::map<int, int> nativeScopeIds;

        ptrMap ptrmap;

};


}

#endif
