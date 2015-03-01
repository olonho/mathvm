#pragma once

#include <vector>
#include <iostream>
#include "../ir.h"

namespace mathvm {
    namespace IR {

        struct TypeDeriver : public IrAnalyzer<VarType, void *> {

            TypeDeriver(std::vector<SimpleIr::VarMeta> const &meta, std::vector<Function *> const &functions, std::ostream &debug)
                    : IrAnalyzer(debug, "TypeDeriver"), meta(meta), functions(functions) {
            }

            virtual VarType visit(const BinOp *const expr) const ;

            virtual VarType visit(const UnOp *const expr) const ;

            virtual VarType visit(const Variable *const expr) const  {
                return meta[expr->id].type;
            }

            virtual VarType visit(const Int *const expr) const  {
                return VarType::VT_Int;
            }

            virtual VarType visit(const Double *const expr) const  {
                return VarType::VT_Double;
            }

            virtual VarType visit(const Ptr *const expr)  const {
                return VarType::VT_Ptr;
            }

            virtual VarType visit(const Call *const expr)  const {
                return functions[expr->funId]->returnType;
            }

            virtual VarType visit(const ReadRef *const expr) const  {
                return meta[expr->refId].type;
            }

        protected:
            virtual VarType defaultAnswer()  const {
                return VT_Unit;
            }

            std::vector<SimpleIr::VarMeta> const &meta;
            std::vector<Function *> const &functions;
        };


    }
}