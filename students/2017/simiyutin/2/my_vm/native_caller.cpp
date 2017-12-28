#include "../my_include/native_caller.h"

void
mathvm::native_caller::call(uint16_t function_id, mathvm::Stack &stack, const std::vector<std::string> &constantStrings,
                            std::map<uint16_t, char *> &dynamicStrings) {
    auto p = nativeFunctions.find(function_id);

    const std::string &function_name = p->second.first;
    const auto &signature = p->second.second;
    auto returnType = signature[0];

    void *func = dlsym(RTLD_DEFAULT, function_name.c_str());

    uint64_t iRegValues[6];
    size_t usedInt = 0;
    double dRegValues[6];
    size_t usedDouble = 0;
    for (size_t i = 1; i < signature.size(); ++i) {
        auto type = signature[i];
        if (type == mathvm::VT_STRING) {
            uint16_t string_id = stack.getUInt16();
            if (string_id < constantStrings.size()) {
                const std::string &constant = constantStrings[string_id];
                const char *data = constant.c_str();
                iRegValues[usedInt++] = (uint64_t) data;
            } else {
                char *data = dynamicStrings[string_id];
                iRegValues[usedInt++] = (uint64_t) data;
            }

        }
        if (type == mathvm::VT_INT) {
            int64_t value = stack.getInt64();
            iRegValues[usedInt++] = (uint64_t) value;
        }
        if (type == mathvm::VT_DOUBLE) {
            double value = stack.getDouble();
            dRegValues[usedDouble++] = value;
        }
    }

    for (size_t i = 0; i < usedInt; ++i) {
        if      (i == 0) asm("movq %0, %%rdi;"::"r" (iRegValues[i]));
        else if (i == 1) asm("movq %0, %%rsi;"::"r" (iRegValues[i]));
        else if (i == 2) asm("movq %0, %%rdx;"::"r" (iRegValues[i]));
        else if (i == 3) asm("movq %0, %%rcx;"::"r" (iRegValues[i]));
        else if (i == 4) asm("movq %0, %%r8;"::"r"  (iRegValues[i]));
        else if (i == 5) asm("movq %0, %%r9;"::"r"  (iRegValues[i]));
    }

    for (size_t i = 0; i < usedDouble; ++i) {
        if      (i == 0) asm("movq %0, %%xmm0;"::"r" (dRegValues[i]));
        else if (i == 1) asm("movq %0, %%xmm1;"::"r" (dRegValues[i]));
        else if (i == 2) asm("movq %0, %%xmm2;"::"r" (dRegValues[i]));
        else if (i == 3) asm("movq %0, %%xmm3;"::"r" (dRegValues[i]));
        else if (i == 4) asm("movq %0, %%xmm4;"::"r" (dRegValues[i]));
        else if (i == 5) asm("movq %0, %%xmm5;"::"r" (dRegValues[i]));
    }

    if (returnType == mathvm::VT_INT || returnType == mathvm::VT_STRING) {
        uint64_t result;
        asm(
        "callq *%1;"
                "movq %%rax, %0;" : "=r" (result) : "r" (func)
        );
        if (returnType == mathvm::VT_INT) {
            stack.addTyped((int64_t) result);
        } else {
            auto* strResult = (char *) result;
            auto string_id = uint16_t(constantStrings.size() + dynamicStrings.size());
            dynamicStrings[string_id] = strResult;
            stack.addTyped(string_id);
        }
    } else if (returnType == mathvm::VT_DOUBLE) {
        double result;
        asm(
        "callq *%1;"
                "movq %%xmm0, %0;" : "=r" (result) : "r" (func)
        );
        stack.addTyped(result);
    } else if (returnType == mathvm::VT_VOID){
        asm("callq *%0;"::"r" (func));
    } else {
        std::cout << "ERROR: INVALID RETURN TYPE OF NATIVE FUNCTION" << std::endl;
        exit(42);
    }

}
