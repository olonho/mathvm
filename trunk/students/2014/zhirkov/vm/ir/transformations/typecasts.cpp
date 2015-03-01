#include <iostream>
#include <sstream>
#include "typecasts.h"
#include "../util.h"
#include "../../exceptions.h"

namespace mathvm {
    namespace IR {

        IrElement *EmitCasts::visit(BinOp const *const expr) {
            auto lt = _deriver.visitExpression(expr->left);
            auto rt = _deriver.visitExpression(expr->right);
            switch (expr->type) {
                case BinOp::BO_ADD:
                case BinOp::BO_SUB:
                case BinOp::BO_MUL:
                case BinOp::BO_DIV:
                case BinOp::BO_LT:
                case BinOp::BO_LE:
                case BinOp::BO_EQ:
                case BinOp::BO_NEQ:
                case BinOp::BO_LOR:
                case BinOp::BO_LAND:
                    if (lt != VT_Double && rt == VT_Double) {
                        auto newlvar = convertTo(VT_Double, expr->left);
                        return new BinOp(new Variable(newlvar), (Atom const *) expr->right->visit(&copier), expr->type);
                    }
                    else if (lt == VT_Double && rt != VT_Double) {
                        auto newrvar = convertTo(VT_Double, expr->right);
                        return new BinOp((Atom const *) expr->left->visit(&copier), new Variable(newrvar), expr->type);
                    }
                    else return base::visit(expr);
                case BinOp::BO_OR:
                case BinOp::BO_AND:
                case BinOp::BO_XOR:
                case BinOp::BO_MOD:
                    if (lt == VT_Double && rt != VT_Double) {
                        auto newlvar = convertTo(VT_Double, expr->left);
                        return new BinOp(new Variable(newlvar), expr->right, expr->type);
                    }
                    else if (lt != VT_Double && rt == VT_Double) {
                        auto newrvar = convertTo(VT_Double, expr->right);
                        return new BinOp(expr->left, new Variable(newrvar), expr->type);
                    }
                    else return base::visit(expr);

                default:
                    _debug << "typechecker can't handle binary operation " << binOpTypeName(expr->type);
                    return NULL;
            };
        }

        IrElement *EmitCasts::visit(UnOp const *const expr) {
            switch (expr->type) {
                case UnOp::UO_NEG:
                    return expr->visit(&copier);
                case UnOp::UO_NOT:
                    if (_deriver.visitExpression(expr->operand) == VT_Double) {
                        auto newvar = convertTo(VT_Int, expr->operand);
                        return new UnOp(new Variable(newvar), expr->type);
                    }
                    else return expr->visit(&copier);
                default:
                    _debug << "typechecker can't handle unary operation " << unOpTypeName(expr->type);
                    return NULL;
            }
        }

        IrElement *EmitCasts::visit(Return const *const expr) {
            if (_currentSourceFunction->returnType == _deriver.visitExpression(expr->atom))
                return base::visit(expr);

            auto newvar = convertTo(_currentSourceFunction->returnType, expr->atom);
            return new Return(new Variable(newvar));
        }

        IrElement *EmitCasts::visit(Assignment const *const expr) {

            SimpleIr::VarMeta &meta = _newIr.varMeta[expr->var->id];
            auto exprType = _deriver.visitExpression(expr->value);
            if (meta.type == exprType) return base::visit(expr);
            if (!meta.isSourceVar) {
                meta.type = exprType;
                return base::visit(expr);
            }
            else {
                auto convertedId = convertTo(meta.type, expr->value);
                return new Assignment(expr->var->id, new Variable(convertedId));
            }
        }

        IrElement *EmitCasts::visit(Call const *const expr) {
            auto f = _oldIr.functions[expr->funId];

            std::vector<const Atom *> params;
            for (size_t i = 0; i < expr->params.size(); i++) {
                const VarType argSigType = _oldIr.varMeta[f->parametersIds[i]].type;
                const VarType argType = _deriver.visitExpression(expr->params[i]);
                if (argType == argSigType)
                    params.push_back(static_cast<Atom *>( expr->params[i]->visit(this) ));
                else {
                    auto e = static_cast<Atom *>( expr->params[i]->visit(this) );
                    params.push_back(new Variable(convertTo(argSigType, e)));
                }
                //todo: reference params need to be converted.??

            }
            return new Call(expr->funId, params, expr->refParams);
        }


        IrElement *EmitCasts::visit(Print const *const expr) {
            return base::visit(expr);
        }

        IrElement *EmitCasts::visit(JumpAlways const *const expr) {
            return base::visit(expr);
        }

        IrElement *EmitCasts::visit(JumpCond const *const expr) {
            return base::visit(expr);
        }

        IrElement *EmitCasts::visit(WriteRef const *const expr) {
            auto refType = _oldIr.varMeta[expr->refId].type;
            auto oldtype = _deriver.visitExpression(expr->atom);
            if (oldtype == refType)
                return base::visit(expr);
            auto id = convertTo(refType, expr->atom);
            return new WriteRef(new Variable(id), expr->refId);
        }

        IrElement *EmitCasts::visit(ReadRef const *const expr) {
            return base::visit(expr);
        }

        VarType TypeDeriver::visit(const BinOp *const expr)  const {
            auto r = IrAnalyzer::visitElement(expr->right);
            auto l = IrAnalyzer::visitElement(expr->left);
            if (l == VT_Error || r == VT_Error) return VT_Error;
            if (l == VT_Undefined || r == VT_Undefined) return VT_Undefined;
            if (l == VT_Ptr || r == VT_Ptr || l == VT_Unit || r == VT_Unit) return VT_Error;

            switch (expr->type) {
                case BinOp::BO_ADD:
                case BinOp::BO_SUB:
                case BinOp::BO_MUL:
                case BinOp::BO_DIV:
                    if (l == VT_Double || r == VT_Double) return VT_Double;
                    return VT_Int;
                case BinOp::BO_MOD:
                case BinOp::BO_AND:
                case BinOp::BO_LE:
                case BinOp::BO_LT:
                case BinOp::BO_LOR:
                case BinOp::BO_EQ:
                case BinOp::BO_NEQ:
                case BinOp::BO_OR:
                case BinOp::BO_LAND:
                case BinOp::BO_XOR:
                    return VT_Int;
                default:
                    return VT_Error;
            }
        }

        VarType TypeDeriver::visit(const UnOp *const expr)  const {

            auto opType = IrAnalyzer::visitElement(expr->operand);
            switch (opType) {
                case VT_Undefined:
                case VT_Error:
                    return opType;
                case VT_Unit:
                    return VT_Error;
                default:
                    switch (expr->type) {
                        case UnOp::UO_NEG:
                            return visitExpression(expr->operand);
                        case UnOp::UO_NOT:
                            return VT_Int;
                        default:
                            return VT_Error;
                    }
            }
        }

        VarId EmitCasts::convertTo(VarType to, Expression const *expr) {

            auto exprType = _deriver.visitExpression(expr);
            auto resId = makeVar(exprType);

            IrTypePrinter printer(_newIr.varMeta, _debug);
            _debug << "converting expression ";
            expr->visit(&printer);
            _debug << " of type " << varTypeStr(exprType) << " to type " << varTypeStr(to) << std::endl;

            emit(new Assignment(resId, (Expression const *const) expr->visit(&copier)));

            if (exprType == to) return resId;
            auto castOp = selectCast(exprType, to);
            if (castOp == UnOp::UO_INVALID) {
                std::stringstream msg;
                msg<< "Can't convert from type " << varTypeStr(exprType) << " to " << varTypeStr(to);
                throw BadIr(msg.str());
            }

            auto convertedId = makeVar(to);
            emit(new Assignment(convertedId, new UnOp(new Variable(resId), castOp)));
            return convertedId;
        }

    }
}