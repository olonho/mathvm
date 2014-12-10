#include <iostream>
#include "ssa.h"
namespace mathvm {
    namespace IR {

        IrElement *SsaTransformation::visit(Variable const *var) {
            if (shouldBeRenamed(var->id))
                return new Variable(_latestVersion[var->id]);
            else return IdentityTransformation::visit(var);
        }

        static void lookupPreviousVariables(uint64_t originId, std::vector<IR::SimpleIr::VarMeta> const& varMeta,  std::set<Variable const* > & phiList, Block const* currentBlock)  {
            for( auto it = currentBlock->contents.rbegin(); it != currentBlock->contents.rend(); ++it)
                if ((*it)->isAssignment()) {
                    auto a = (*it)->asAssignment();
                    auto lhs = a->var->id;
                    if (varMeta[lhs].isSourceVar && varMeta[lhs].originId == originId) {
                        phiList.insert(new Variable(lhs));
                        return;
                    }
                }
            for (auto pred : currentBlock->predecessors)
                lookupPreviousVariables(originId, varMeta, phiList, pred);
        }

        IrElement *SsaTransformation::visit(Phi const *const expr) {
            auto oldVarId = expr->var->id;

            if (_currentIr->varMeta[oldVarId].isSourceVar) {
                uint64_t newId = meta.size();

                Variable const *const newLhs = new Variable(newId);

                SimpleIr::VarMeta newmeta(newId, oldVarId, meta[oldVarId].type);
                meta.push_back(newmeta);
                _latestVersion[oldVarId] = newId;
                Phi * result = new Phi(newLhs);
                lookupPreviousVariables(oldVarId, meta, result->vars, _currentSourceBlock);
                return result;
            }
            return IdentityTransformation::visit(expr);
        }

        IrElement *SsaTransformation::visit(Assignment const *const expr) {
            auto oldVarId = expr->var->id;

            if (meta[oldVarId].isSourceVar) {
                uint64_t newId = meta.size();

                Variable const *const newLhs = new Variable(newId);
                Expression const *const newrhs = static_cast<Expression const *> (expr->value->visit(this));

                SimpleIr::VarMeta newmeta(newId, oldVarId, meta[oldVarId].type);
                meta.push_back(newmeta);
                _latestVersion[oldVarId] = newId;
                return new Assignment(newLhs, newrhs);
            }
            return IdentityTransformation::visit(expr);
        }


        IrElement *SsaTransformation::visit(Int const *const expr) {
            return IdentityTransformation::visit(expr);
        }

        IrElement *SsaTransformation::visit(Double const *const expr) {
            return IdentityTransformation::visit(expr);
        }

        IrElement *SsaTransformation::visit(Ptr const *const expr) {
            return IdentityTransformation::visit(expr);
        }

        IrElement *SsaTransformation::visit(Block const *const expr) {
            return IdentityTransformation::visit(expr);
        }


        IrElement *SsaTransformation::visit(Call const *const expr) {
            return IdentityTransformation::visit(expr);
        }

        IrElement *SsaTransformation::visit(Print const *const expr) {
            return IdentityTransformation::visit(expr);
        }

        IrElement *SsaTransformation::visit(FunctionRecord const *const expr) {
            return IdentityTransformation::visit(expr);
        }

        IrElement *SsaTransformation::visit(JumpAlways const *const expr) {
            return IdentityTransformation::visit(expr);
        }

        IrElement *SsaTransformation::visit(JumpCond const *const expr) {
            return IdentityTransformation::visit(expr);
        }


        IrElement *SsaTransformation::visit(BinOp const *const expr) {
            return IdentityTransformation::visit(expr);
        }

        IrElement *SsaTransformation::visit(UnOp const *const expr) {
            return IdentityTransformation::visit(expr);
        }

        IrElement *SsaTransformation::visit(Return const *const expr) {
            return IdentityTransformation::visit(expr);
        }

        IrElement *SsaTransformation::visit(WriteRef const *const expr) {

            auto e = expr->atom->visit(this);
            if (!e) return NULL;
            return new WriteRef((Atom const *const) e, expr->refId);
        }

        IrElement *SsaTransformation::visit(ReadRef const *const expr) {
            return IdentityTransformation::visit(expr);
        }
    }
}