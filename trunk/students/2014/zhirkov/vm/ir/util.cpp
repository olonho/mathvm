#include "util.h"

namespace mathvm {
    namespace IR {

        char const* typeName(VarType type) {
            switch(type) {

                case VT_Bot: return "_|_";
                case VT_Int: return "int";
                case VT_Double: return "double";
                case VT_Ptr: return "ptr";
            }
            return "INVALID_TYPE";
        }
        
    }
}