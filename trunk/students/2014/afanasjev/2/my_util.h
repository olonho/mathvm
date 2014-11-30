#pragma once
#include "string"
#include "mathvm.h"
#include "ast.h"
#include <dlfcn.h>

namespace mathvm {
inline void* locateNativeFunction(std::string const & name) {
    void* fptr = dlsym(RTLD_DEFAULT, name.c_str());

    if(dlerror() != NULL) {
        return 0;
    }
    return fptr;
}

inline VarType getWidestType(VarType first, VarType second) {
    if(first == second) {
        return first;
    }

    if((first == VT_INT && second == VT_DOUBLE) ||
            (first == VT_DOUBLE && second == VT_INT)) {
        return VT_DOUBLE;
    }
    else {
        return VT_INVALID;
    }
}

inline bool canCast(VarType from, VarType to) {
    if(from == to) {
        return true;
    }

    if((from == VT_INT || from == VT_DOUBLE) &&
       (to == VT_INT || to == VT_DOUBLE)) {
        return true;
    } 

    return false;
}

inline bool isFunctionNative(FunctionNode* node) {
    return node->body()->nodes() > 0 && 
        node->body()->nodeAt(0)->isNativeCallNode();
}

inline bool isFunctionNative(AstFunction* function) {
    FunctionNode* node = function->node(); 
    return isFunctionNative(node);
}

}
