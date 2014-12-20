#pragma once

#include <ostream>

#include "ir.h"
#include <set>
namespace mathvm {
    namespace IR {
        char const* typeName(VarType type) ;
        std::set<uint64_t> modifiedVars(const Block* const block );
        char const* binOpTypeName(BinOp::Type type);
        char const* unOpTypeName(UnOp::Type type);

    }
}