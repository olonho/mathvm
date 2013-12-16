#include "machcode.h"

#define STACK_SIZE 16 * 1024 * 1024

using namespace mathvm;

Status *AsmJitCodeImpl::execute(vector<Var *> &vars) {
    VoidFunc main = AsmJit::function_cast<VoidFunc>(funcPtrs[0]);
    uint32_t mainVars = functionById(0)->varsToStore();

    char *funcStack = (char*)malloc(STACK_SIZE);
    main(funcStack, funcStack + sizeof(int64_t) * mainVars);
    free(funcStack);
    return 0;
}
