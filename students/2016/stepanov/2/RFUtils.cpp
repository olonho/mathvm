//
// Created by user on 11/23/16.
//

#include "RFUtils.h"

namespace mathvm {
    // VT_INVALID = 0,  VT_VOID, VT_DOUBLE, VT_INT, VT_STRING
    char typeMangling[] = {'Z', 'V', 'D', 'I', 'S'};

    std::string manglingName(const Signature &signature) {
        std::string s = "$";
        for (size_t i = 1; i < signature.size(); ++i){
            s.push_back(typeMangling[signature[i].first]);
        }
        return s;
    }
}