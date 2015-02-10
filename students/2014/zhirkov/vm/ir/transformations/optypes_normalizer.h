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

        struct OperationTypesNormalizer : public Transformation<> {

            virtual ~OperationTypesNormalizer() {
            }

            virtual void operator()() {
                Transformation::visit(&_oldIr);
            }

            TypeDeriver _deriver;

            virtual IrElement *visit(const UnOp *const expr) {
                auto newtype = floatType(expr->type);
                if (_deriver.visitExpression(expr->operand) == VT_Double)
                    return new UnOp((Atom const *) expr->operand->visit(this), newtype);
                else return BaseTransform::visit(expr);
            }

            virtual IrElement *visit(const BinOp *const expr) {
                auto newtype = floatType(expr->type);
                if (_deriver.visitExpression(expr->left) == VT_Double || _deriver.visitExpression(expr->right) == VT_Double)
                    return new BinOp((Atom const *) expr->left->visit(this), (Atom const *) expr->right->visit(this), newtype);
                else return BaseTransform::visit(expr);
            }

            OperationTypesNormalizer(SimpleIr const &source, SimpleIr &dest, ostream &_debug)
                    : Transformation(source, dest, "operation types normalizer", _debug),
                      _deriver(_newIr.varMeta, source.functions, _debug) {
            }

            static UnOp::Type floatType(UnOp::Type old) {
                switch (old) {
                    case UnOp::UO_NEG:
                        return UnOp::UO_FNEG;
                    default:
                        return old;
                }
            }

            static BinOp::Type floatType(BinOp::Type old) {
                switch (old) {
                    case BinOp::BO_ADD:
                        return BinOp::BO_FADD;
                    case BinOp::BO_SUB:
                        return BinOp::BO_FSUB;
                    case BinOp::BO_MUL:
                        return BinOp::BO_FMUL;
                    case BinOp::BO_DIV:
                        return BinOp::BO_FDIV;
                    case BinOp::BO_LT:
                        return BinOp::BO_FLT;
                    case BinOp::BO_LE:
                        return BinOp::BO_FLE;
                    default:
                        return old;
                }

            }
        };

    }
}
