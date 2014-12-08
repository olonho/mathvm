#pragma once

#include <set>
#include "ir.h"

namespace mathvm {
    namespace IR {


        inline char const *const varTypeStr(VarType type) {
            switch (type) {
                case VT_Bot:
                    return "_|_";
                case VT_Int:
                    return "int";
                case VT_Double:
                    return "double";
                case VT_Ptr:
                    return "ptr";
                case VT_Undefined:
                    return "???";
            }
            return "check varTypeStr, no case";
        }

        class IrPrinter : public IrVisitor {
            std::ostream &_out;
            std::set<const Block *> visitedBlocks;
            FunctionRecord const* currentFunction;
            bool visited(const Block *const block)  {
                return visitedBlocks.find(block) != visitedBlocks.end();
            }

        public:
            FOR_IR(VISITOR)

            IrPrinter(std::ostream &out) : _out(out) , currentFunction(NULL) {
            }
            void print(SimpleIr const& ir);
        };


    }
}