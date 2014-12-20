#include "util.h"

namespace mathvm {
    namespace IR {

        char const *typeName(VarType type) {
            switch (type) {
                case VT_Unit:
                    return "unit";
                case VT_Int:
                    return "int";
                case VT_Double:
                    return "double";
                case VT_Ptr:
                    return "ptr";
                case VT_Undefined:
                    return "???";
                default:
                    return "ErrorType";
            }
        }

        std::set<uint64_t> modifiedVars(const Block *const block) {
            std::set<uint64_t> result;
            for (auto st : block->contents) {
                if (st->isAssignment())
                    result.insert(st->asAssignment()->var->id);
            }
            return result;
        }

        char const *binOpTypeName(BinOp::Type type) {
#define NAME(n, _) case BinOp::BO_##n: return #n;
            switch (type) {
                FOR_IR_BINOP(NAME)
                default:
                    return "???";
            }
#undef NAME
        }

        char const *unOpTypeName(UnOp::Type type) {
#define NAME(n, _) case UnOp::UO_##n: return #n;
            switch (type) {
                FOR_IR_UNOP(NAME)
                default:
                    return "???";
            }
#undef NAME
        }
    }
}