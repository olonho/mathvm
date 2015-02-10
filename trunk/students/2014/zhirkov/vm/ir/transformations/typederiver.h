#pragma once

#include <vector>
#include <iostream>
#include "../ir.h"

namespace mathvm {
    namespace IR {

        struct TypeDeriver : public IrAnalyzer<VarType, void *> {

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


    }
}