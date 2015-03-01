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

        class IrPrinter : public IrVisitor<void> {
        protected:
            std::ostream &_out;
            Function const* currentFunction;

        public:
            FOR_IR(VISITOR_VOID)

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

            virtual void visit(Variable const *const expr) {
                auto& m = meta[expr->id];
                _out << (m.isSourceVar ?  ("[var ") :( "[tmp "));
                _out << expr->id  << ":" << varTypeStr(m.type) << "]";
            }

        };


        inline std::ostream& operator<<(std::ostream& s, IR::IrElement const& elem) {
            IrPrinter printer(s);
            elem.visit(&printer);
            return s;
        }

    }
}