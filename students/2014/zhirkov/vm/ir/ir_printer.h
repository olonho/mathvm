#pragma once

#include <set>
#include "ir.h"

namespace mathvm {
    namespace IR {


        inline char const *const varTypeStr(VarType type) {
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
                case VT_Error:
                    return "errortype";
            }
            return "check varTypeStr, no case";
        }

        class IrPrinter : public IrVisitor {
        protected:
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

        class IrTypePrinter : public IrPrinter {
        private:
            std::vector<SimpleIr::VarMeta> const& meta;
        public:

            IrTypePrinter(std::vector<SimpleIr::VarMeta> const& meta, std::ostream &out) : IrPrinter(out) , meta(meta) {
            }

            virtual IrElement *visit(Variable const *const expr) {
                auto& m = meta[expr->id];
                _out << (m.isSourceVar ?  ("[var ") :( "[tmp "));
                _out << expr->id  << ":" << varTypeStr(m.type) << "]";
                return NULL;
            }

        };


    }
}