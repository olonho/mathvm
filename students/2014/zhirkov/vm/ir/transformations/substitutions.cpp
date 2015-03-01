#include "substitutions.h"
#include "../ir_printer.h"

namespace mathvm {
    namespace IR {

        IrElement *Substitution::visit(Variable const *const expr) {
            if (canSubstitute(expr->id)) {
                IrElement *res = _substitutions[expr->id]->visit(&copier);
                _debug << "! substituted " << *expr << " -> " << *res  << std::endl;
                _changed = true;
                return res;
            }
            else return copier.visit(expr);
        }

        IrElement *Substitution::visit(Phi const *const expr) {
            if (!isUsed(expr->var->id) && _oldIr.varMeta[expr->var->id].type != VT_Unit) {
                _debug << "dead phi function will be removed: " << *expr << std::endl;
                _changed = true;
                return NULL;
            }
            if (expr->vars.size() == 1)
            {
                _changed = true;
                return new Assignment(expr->var->id, new Variable((*(expr->vars.begin()))->id));
            }
            return copier.visit(expr);
        }

        IrElement *Substitution::visit(Assignment const *const expr) {
            if (!isUsed(expr->var->id) && _oldIr.varMeta[expr->var->id].type != VT_Unit ) {
                _debug << "dead assignment will be removed: " << *expr << std::endl;
                _changed = true;
                return NULL;
            }
            Expression const *const res = (Expression const *const) expr->value->visit(this);
            if (expr->value->isAtom() && ! _oldIr.varMeta[expr->var->id].isReference) {
                _debug << "will substitute " << *expr << " -> " << *res << std::endl;
                _substitutions[expr->var->id] = res->asAtom();
                _changed = true;
            }
            return new Assignment(expr->var->id, res);
        };

        IrElement *Substitution::visit(BinOp const *const expr) {
            if (!expr->left->isLiteral() || !expr->right->isLiteral()) return base::visit(expr);

#define CALC(ltype, rtype, restype, act) if (expr->left->is##ltype() && expr->right->is##rtype()) { _changed = true; return new restype(expr->left->as##ltype()->value act expr->right->as##rtype()->value); }
            _changed = true;
            switch (expr->type) {
                case BinOp::BO_ADD: {
                    CALC(Int, Int, Int, +)
                    break;
                };
                case BinOp::BO_SUB: {
                    CALC(Int, Int, Int, -)
                    break;
                }
                case BinOp::BO_MUL: {
                    CALC(Int, Int, Int, *)
                    break;
                }
                case BinOp::BO_DIV: {
                    CALC(Int, Int, Int, /)
                    break;
                }
                case BinOp::BO_MOD: {
                    CALC(Int, Int, Int, %)
                    break;
                }
                case BinOp::BO_LT: {
                    CALC(Int, Int, Int, <)
                    break;
                }
                case BinOp::BO_LE: {
                    CALC(Int, Int, Int, <=)
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
                case BinOp::BO_FADD: {
                    CALC(Double, Double, Double, +)
                    break;
                };
                case BinOp::BO_FSUB: {
                    CALC(Int, Int, Int, -)
                    CALC(Double, Double, Double, -)
                    break;
                }
                case BinOp::BO_FMUL: {
                    CALC(Double, Double, Double, *)
                    break;
                };
                case BinOp::BO_FDIV: {
                    CALC(Double, Double, Double, /)
                    break;
                };
                case BinOp::BO_FLT: {
                    CALC(Double, Double, Double, <)
                    break;
                }
                case BinOp::BO_FLE: {
                    CALC(Double, Double, Double, <=)
                    break;
                }
                default:
                    break;

            };
            return base::visit(expr);
#undef CALC
        }

        IrElement *Substitution::visit(UnOp const *const expr) {
            if (!expr->operand->isLiteral()) return base::visit(expr);
            switch (expr->type) {
                case UnOp::UO_CAST_I2D:
                    _changed = true;
                    return new Double(expr->operand->asInt()->value);
                case UnOp::UO_CAST_D2I:
                    _changed = true;
                    return new Int(expr->operand->asDouble()->value);
                case UnOp::UO_NEG:
                    _changed = true;
                    return new Int(-expr->operand->asInt()->value);

                case UnOp::UO_FNEG:
                    _changed = true;
                    return new Double(-expr->operand->asDouble()->value);
                case UnOp::UO_NOT:
                    if (expr->operand->isInt()) {
                        _changed = true;
                        return new Int(!expr->operand->asInt()->value);
                    }
                    else if (expr->operand->isDouble()) {
                        _changed = true;
                        return new Int(expr->operand->asDouble()->value == 0.0);
                    }
                    break;

                default:
                    break;
            }
            return base::visit(expr);
        }

        bool Substitution::operator()() {
            Transformation::visit(&_oldIr);
            return isTrivial();
        }


        IrElement *Substitution::visit(Return const *const expr) {
            if (expr->atom->isVariable()) {
                auto varid = expr->atom->asVariable()->id;
                if (canSubstitute(varid)) {
                    IR::Return* const res = new Return((Atom const *const) _substitutions[varid]->visit(&copier));

                    _debug << "! substituted " << *expr << " -> " << *res;

                    _changed = true;
                    return res;
                }
                else return copier.visit(expr);
            }
            else return copier.visit(expr);
        }
    }
}