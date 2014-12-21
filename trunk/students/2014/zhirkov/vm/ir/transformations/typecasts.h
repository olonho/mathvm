#pragma once

#include <map>
#include <set>
#include "identity.h"
#include "../ir_printer.h"
#include "../ir_analyzer.h"
#include "../../ast_analyzer.h"


namespace mathvm {
    namespace IR {

        struct TypeDeriver : public IrAnalyzer<VarType, void*> {

            TypeDeriver(std::vector<SimpleIr::VarMeta> const &meta, std::vector<FunctionRecord *> const &functions, std::ostream &debug)
                    : IrAnalyzer(debug, "TypeDeriver"), meta(meta), functions(functions) {
            }

            virtual VarType visit(const BinOp *const expr);

            virtual VarType visit(const UnOp *const expr);

            virtual VarType visit(const Variable *const expr) {
                return meta[expr->id].type;
            }

            virtual VarType visit(const Int *const expr) {
                return VarType::VT_Int;
            }

            virtual VarType visit(const Double *const expr) {
                return VarType::VT_Double;
            }

            virtual VarType visit(const Ptr *const expr) {
                return VarType::VT_Ptr;
            }

            virtual VarType visit(const Call *const expr) {
                return functions[expr->funId]->returnType;
            }

            virtual VarType visit(const ReadRef *const expr) {
                return meta[expr->refId].type;
            }

        protected:
            virtual VarType defaultAnswer() {
                return VT_Unit;
            }

            std::vector<SimpleIr::VarMeta> const &meta;
            std::vector<FunctionRecord *> const &functions;
        };


        class EmitCasts : public Transformation {

        public:
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

            EmitCasts(SimpleIr const *old, std::ostream &debug = std::cerr)
                    : Transformation(old, "typechecker", debug),
                      _deriver(_currentIr->varMeta, old->functions, debug){
            }

        private:
            TypeDeriver _deriver;

            static UnOp::Type selectCast(VarType from, VarType to) {
                if (from == VT_Error || to == VT_Error) return UnOp::UO_INVALID;
                if ((from == VT_Int || from == VT_Ptr) && to == VT_Double) return UnOp::UO_CAST_I2D;
                if (from == VT_Double && to == VT_Int) return UnOp::UO_CAST_D2I;
                return UnOp::UO_INVALID;
            }

            uint64_t convertTo(VarType to, Expression const *expr) {

                auto exprType = _deriver.visitExpression(expr);
                auto resId = makeVar(exprType);

                IrTypePrinter printer(_currentIr->varMeta,_debug);
                _debug << "converting expression ";
                expr->visit(&printer);
                _debug << " of type " << varTypeStr(exprType) << " to type " << varTypeStr(to) << std::endl;

                emit(new Assignment(resId, (Expression const *const) expr->visit(&copier)));

                if (exprType == to) return resId;
                auto castOp = selectCast(exprType, to);
                if (castOp == UnOp::UO_INVALID) {
                    _debug << "Can't convert from type " << varTypeStr(exprType) << " to " << varTypeStr(to) << std::endl;
                    return resId;
                }

                auto convertedId = makeVar(to);
                emit(new Assignment(convertedId, new UnOp(new Variable(resId), castOp)));
                return convertedId;
            }

        };
    }
}
