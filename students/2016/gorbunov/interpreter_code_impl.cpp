#include "interpreter_code_impl.h"

#include <dlfcn.h>
#include <bytecode_interpreter.h>

#include "mathvm_error.h"

using namespace mathvm;

Status *InterpreterCodeImpl::execute(vector<Var*> &vars) {
    auto var_it = Scope::VarIterator(_top_scope);

    vector<int32_t> var_ids(vars.size(), -1);
    int32_t id = 0;
    while (var_it.hasNext()) {
        auto ast_var = var_it.next();
        for (uint32_t i = 0; i < vars.size(); ++i) {
            if (vars[i]->name() == ast_var->name()) {
                var_ids[i] = id;
                break;
            }
        }
        id += 1;
    }

    auto vm = BytecodeInterpreter(this, std::cout);

    for (uint32_t i = 0; i < vars.size(); ++i) {
        switch (vars[i]->type()) {
            case VT_INVALID:
            case VT_VOID:
                throw InterpreterError("bad init vars type");
            case VT_DOUBLE:
                vm.set_topmost_var_by_id(var_ids[i], vars[i]->getDoubleValue());
                break;
            case VT_INT:
                vm.set_topmost_var_by_id(var_ids[i], vars[i]->getIntValue());
                break;
            case VT_STRING:
                vm.set_topmost_var_by_id(var_ids[i], vars[i]->getStringValue());
                break;
        }
    }

    auto status = vm.run();
    if (status->isError()) {
        return status;
    }

    for (uint32_t i = 0; i < vars.size(); ++i) {
        switch (vars[i]->type()) {
            case VT_INVALID:
            case VT_VOID:
                throw InterpreterError("bad init vars type");
            case VT_DOUBLE:
                vars[i]->setDoubleValue(vm.get_topmost_var_by_id<double>(var_ids[i]));
                break;
            case VT_INT:
                vars[i]->setIntValue(vm.get_topmost_var_by_id<int64_t>(var_ids[i]));
                break;
            case VT_STRING:
                vars[i]->setStringValue(vm.get_topmost_var_by_id<const char*>(var_ids[i]));
                break;
        }
    }



    return status;
}

uint16_t InterpreterCodeImpl::registerNativeFunction(BytecodeFunction *bf) {
    throw TranslatorError("Native functions not implemented");
    return 0;
}
