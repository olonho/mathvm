#include "substitutions.h"

namespace mathvm {
    namespace IR {

        IrElement *Substitution::visit(Variable const *const expr) {
            if (_substitutions.find(expr->id) != _substitutions.end()) {
                IrPrinter printer(_debug);
                _debug << "! substituted ";
                expr->visit(&printer);
                _debug << " -> ";
                IrElement *res = _substitutions[expr->id]->visit(&copier);
                res->visit(&printer);
                _debug << std::endl;
                _changed = true;
                return res;
            }
            else return copier.visit(expr);
        }

        IrElement *Substitution::visit(Assignment const *const expr) {
            Expression const *const res = (Expression const *const) expr->value->visit(this);
            if (expr->value->isAtom()) {
                IrPrinter printer(_debug);
                _debug << "will substitute ";
                expr->var->visit(&printer);
                _debug << " -> ";
                res->visit(&printer);
                _debug << std::endl;
                _substitutions[expr->var->id] = res->asAtom();
            }
            return new Assignment(expr->var->id, res);
        };

        IrElement *Substitution::visit(BinOp const *const expr) {
            if (!expr->left->isLiteral() || !expr->right->isLiteral()) return Transformation::visit(expr);

#define CALC(ltype, rtype, restype, act) if (expr->left->is##ltype() && expr->right->is##rtype()) { _changed = true; return new restype(expr->left->as##ltype()->value act expr->right->as##rtype()->value); }
            _changed = true;
            switch (expr->type) {
                case BinOp::BO_ADD: {
                    CALC(Int, Int, Int, +)
                    CALC(Double, Double, Double, +)
                    break;
                };
                case BinOp::BO_SUB: {
                    CALC(Int, Int, Int, -)
                    CALC(Double, Double, Double, -)
                    break;
                }
                case BinOp::BO_MUL: {
                    CALC(Int, Int, Int, *)
                    CALC(Double, Double, Double, *)
                    break;
                }
                case BinOp::BO_DIV: {
                    CALC(Int, Int, Int, /)
                    CALC(Double, Double, Double, /)
                    break;
                }
                case BinOp::BO_MOD: {
                    CALC(Int, Int, Int, %)
                    break;
                }
                case BinOp::BO_LT: {
                    CALC(Int, Int, Int, <)
                    CALC(Double, Double, Double, <)
                    break;
                }
                case BinOp::BO_LE: {
                    CALC(Int, Int, Int, <=)
                    CALC(Double, Double, Double, <=)
                    break;
                }
                case BinOp::BO_EQ: {
                    CALC(Int, Int, Int, ==)
                    CALC(Double, Double, Double, ==)
                    break;
                }
                case BinOp::BO_NEQ: {
                    CALC(Int, Int, Int, !=)
                    CALC(Double, Double, Double, !=)
                    break;
                }
                case BinOp::BO_OR: {
                    CALC(Int, Int, Int, |)
                    break;
                }
                case BinOp::BO_AND: {
                    CALC(Int, Int, Int, &)
                    break;
                }
                case BinOp::BO_LOR: {
                    CALC(Int, Int, Int, ||)
                    break;
                }
                case BinOp::BO_LAND: {
                    CALC(Int, Int, Int, &&)
                    break;
                }
                case BinOp::BO_XOR: {
                    CALC(Int, Int, Int, ^)
                    break;
                }
                default:break;
            }
            return Transformation::visit(expr);
#undef CALC
        }

        IrElement *Substitution::visit(UnOp const *const expr) {
            if (!expr->operand->isLiteral()) return Transformation::visit(expr);
            switch (expr->type) {
                case UnOp::UO_CAST_I2D:
                   _changed = true; return new Double(expr->operand->asInt()->value);
                case UnOp::UO_CAST_D2I:
                    _changed = true;  return new Int(expr->operand->asInt()->value);
                case UnOp::UO_NEG:
                    if (expr->operand->isInt()) { _changed = true;  return new Int(-expr->operand->asInt()->value);}
                    if (expr->operand->isDouble()) {_changed = true; return new Double(-expr->operand->asInt()->value);}
                    break;
                case UnOp::UO_NOT:
                    if (expr->operand->isInt()) {_changed = true;  return new Int(!expr->operand->asInt()->value);}
                    break;
                default:
                    break;
            }
            return Transformation::visit(expr);
        }

//gotta delete dead guys
        bool Referenced::visit(const BinOp *const expr) {
            return IrAnalyzer::visit(expr);
        }

        bool Referenced::visit(const UnOp *const expr) {
            return IrAnalyzer::visit(expr);
        }

        bool Referenced::visit(const Variable *const expr) {
            return IrAnalyzer::visit(expr);
        }

        bool Referenced::visit(const Return *const expr) {
            return IrAnalyzer::visit(expr);
        }

        bool Referenced::visit(const Phi *const expr) {
            return IrAnalyzer::visit(expr);
        }

        bool Referenced::visit(const Int *const expr) {
            return IrAnalyzer::visit(expr);
        }

        bool Referenced::visit(const Double *const expr) {
            return IrAnalyzer::visit(expr);
        }

        bool Referenced::visit(const Ptr *const expr) {
            return IrAnalyzer::visit(expr);
        }

        bool Referenced::visit(const Block *const expr) {
            return IrAnalyzer::visit(expr);
        }

        bool Referenced::visit(const Assignment *const expr) {
            return IrAnalyzer::visit(expr);
        }

        bool Referenced::visit(const Call *const expr) {
            return IrAnalyzer::visit(expr);
        }

        bool Referenced::visit(const Print *const expr) {
            return IrAnalyzer::visit(expr);
        }

        bool Referenced::visit(const FunctionRecord *const expr) {
            return IrAnalyzer::visit(expr);
        }

        bool Referenced::visit(const JumpAlways *const expr) {
            return IrAnalyzer::visit(expr);
        }

        bool Referenced::visit(const JumpCond *const expr) {
            return IrAnalyzer::visit(expr);
        }

        bool Referenced::visit(const WriteRef *const expr) {
            return IrAnalyzer::visit(expr);
        }

        bool Referenced::visit(const ReadRef *const expr) {
            return IrAnalyzer::visit(expr);
        }
    }
}