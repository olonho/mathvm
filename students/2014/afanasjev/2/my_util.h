#pragma once
#include "string"
#include "mathvm.h"

namespace mathvm {
inline void* locateNativeFunction(std::string const & name) {
    return 0;
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
       (to == VT_INT || to == VT_INT)) {
        return true;
    } 

    return false;
}

}
