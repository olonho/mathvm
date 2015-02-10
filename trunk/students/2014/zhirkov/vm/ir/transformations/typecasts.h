#pragma once

#include <map>
#include <set>
#include "identity.h"
#include "../ir_printer.h"
#include "../ir_analyzer.h"
#include "../../ast_analyzer.h"
#include "typederiver.h"

namespace mathvm {
    namespace IR {


        class EmitCasts : public Transformation<> {

        public:
            virtual void operator()() {
                Transformation::visit(&_oldIr);
            }

            virtual IrElement *visit(BinOp const *const expr);

            virtual IrElement *visit(UnOp const *const expr);

            virtual IrElement *visit(Return const *const expr);

            virtual IrElement *visit(Assignment const *const expr);

            virtual IrElement *visit(Call const *const expr);

            virtual IrElement *visit(Print const *const expr);

            virtual IrElement *visit(JumpAlways const *const expr);

            virtual IrElement *visit(JumpCond const *const expr);

            virtual IrElement *visit(WriteRef const *const expr);

            virtual IrElement *visit(ReadRef const *const expr);

            EmitCasts(SimpleIr const &source, SimpleIr &dest, std::ostream &debug = std::cerr)
                    : Transformation(source, dest, "typechecker", debug),
                      _deriver(_newIr.varMeta, source.functions, debug) {
            }

        private:
            TypeDeriver _deriver;

            static UnOp::Type selectCast(VarType from, VarType to) {
                if (from == VT_Error || to == VT_Error) return UnOp::UO_INVALID;
                if ((from == VT_Int || from == VT_Ptr) && to == VT_Double) return UnOp::UO_CAST_I2D;
                if (from == VT_Double && to == VT_Int) return UnOp::UO_CAST_D2I;
                return UnOp::UO_INVALID;
            }

            VarId convertTo(VarType to, Expression const *expr);
        };
    }
}
