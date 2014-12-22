#include <iostream>
#include "ssa.h"
namespace mathvm {
    namespace IR {

        IrElement *SsaTransformation::visit(Variable const *const var) {
            if (shouldBeRenamed(var->id))
                return new Variable(_latestVersion[var->id]);
            else return Transformation::visit(var);
        }

        static void lookupPreviousVariables(VarId originId, std::vector<IR::SimpleIr::VarMeta> const& varMeta,  std::set<VarId> & phiList, Block const* currentBlock)  {
            for( auto it = currentBlock->contents.rbegin(); it != currentBlock->contents.rend(); ++it)
                if ((*it)->isAssignment()) {
                    auto a = (*it)->asAssignment();
                    auto lhs = a->var->id;
                    if (varMeta[lhs].isSourceVar && varMeta[lhs].originId == originId) {
                        phiList.insert(lhs);
                        return;
                    }
                }
            for (auto pred : currentBlock->predecessors)
                lookupPreviousVariables(originId, varMeta, phiList, pred);
        }

        IrElement *SsaTransformation::visit(Phi const *const expr) {
            auto oldVarId = expr->var->id;

            if (_currentIr->varMeta[oldVarId].isSourceVar) {
                VarId newId = meta.size();

                Variable const *const newLhs = new Variable(newId);

                SimpleIr::VarMeta newmeta(newId, oldVarId, meta[oldVarId].type);
                meta.push_back(newmeta);
                _latestVersion[oldVarId] = newId;
                Phi * result = new Phi(newLhs);
                std::set<VarId> varids;
                lookupPreviousVariables(oldVarId, meta, varids, _currentSourceBlock);
                for (auto id : varids)
                    result->vars.insert(new Variable(id));
                return result;
            }
            return Transformation::visit(expr);
        }

        IrElement *SsaTransformation::visit(Assignment const *const expr) {
            auto oldVarId = expr->var->id;

            if (meta[oldVarId].isSourceVar) {
                VarId newId = meta.size();

                Variable const *const newLhs = new Variable(newId);
                Expression const *const newrhs = static_cast<Expression const *> (expr->value->visit(this));

                SimpleIr::VarMeta newmeta(newId, oldVarId, meta[oldVarId].type);
                meta.push_back(newmeta);
                _latestVersion[oldVarId] = newId;
                return new Assignment(newLhs, newrhs);
            }
            return Transformation::visit(expr);
        }


        IrElement *SsaTransformation::visit(Call const *const expr) {
            std::vector<const Atom*> newparams;
            for (auto p : expr->params)
                newparams.push_back((Atom const *) p->visit(this));

            return new Call(expr->funId, newparams, expr->refParams);
        }

        IrElement *SsaTransformation::visit(FunctionRecord const *const expr) {
            FunctionRecord *transformed = new FunctionRecord(expr->id, expr->returnType, NULL);

            for (auto p : expr->parametersIds)
                transformed->parametersIds.push_back((shouldBeRenamed(p))? newName(p):p);
            for (auto p : expr->memoryCells)
                transformed->memoryCells.push_back((shouldBeRenamed(p))? newName(p):p);
            for (auto p : expr->refParameterIds)
                transformed->refParameterIds.push_back((shouldBeRenamed(p))? newName(p):p);
            Block* newEntry = static_cast<Block*> ( expr->entry->visit(this) );
            if (newEntry == NULL) { delete transformed; return NULL; }
            transformed->entry = newEntry;
            return transformed;
        }

        IrElement *SsaTransformation::visit(WriteRef const *const expr) {

            auto e = expr->atom->visit(this);
            if (!e) return NULL;
            return new WriteRef((Atom const *const) e, expr->refId);
        }

        IrElement *SsaTransformation::visit(ReadRef const *const expr) {
            if (shouldBeRenamed(expr->refId))
                return new ReadRef(_latestVersion[expr->refId]);
            return Transformation::visit(expr);
        }
    }
}