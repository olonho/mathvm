#pragma once
#include <vector>
#include <set>
#include <map>
#include "../ir/ir.h"
#include "../../../../../include/mathvm.h"

namespace mathvm {

    typedef std::map<IR::Block const *, std::set<IR::Block const *>> Dominators;


    std::vector<const IR::Block*> blocksPostOrder(const IR::Block* const startBlock);

    std::set<const IR::Block *> collectPredecessors(IR::Block * block);

    Dominators dominators(const IR::Block *const startBlock);
    std::map<const IR::Block *, const IR::Block *> immediateDominators(const IR::Block *const startBlock);

    std::map<const IR::Block *, std::set<const IR::Block *>> dominanceFrontier(const IR::Block *const startBlock);

    inline IR::VarType vtToIrType(VarType type) {
        switch (type) {
            case VT_DOUBLE:
                return IR::VT_Double;
            case VT_INT:
                return IR::VT_Int;
            case VT_STRING:
                return IR::VT_Ptr;
            case VT_VOID:
                return IR::VT_Unit;
            case VT_INVALID:
                return IR::VT_Error;
            default:
                return IR::VT_Error;
        }
    }
}