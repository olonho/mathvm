#include <iostream>
#include "ssa.h"
#include "../../../../../../include/mathvm.h"

namespace mathvm {
    namespace IR {

        IrElement *Ssa::visit(Variable const *const var) {
            if (shouldBeRenamed(var->id))
                return new Variable(_latestVersion[var->id]);
            else return base::visit(var);
        }

        IrElement *Ssa::visit(Phi const *const expr) {
            auto oldVarId = expr->var->id;

            if (_newIr.varMeta[oldVarId].isSourceVar) {
                VarId newId = meta.size();

                SimpleIr::VarMeta newmeta(newId, oldVarId, meta[oldVarId].type);
                meta.push_back(newmeta);
                _latestVersion[oldVarId] = newId;
                return new Phi(newId); //expect phi to be empty at that stage, ssa should come before phi_values transformation
            }
            return base::visit(expr);
        }

        IrElement *Ssa::visit(Assignment const *const expr) {
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
            return base::visit(expr);
        }


        IrElement *Ssa::visit(Call const *const expr) {
            std::vector<const Atom *> newparams;
            for (auto p : expr->params)
                newparams.push_back((Atom const *) p->visit(this));

            return new Call(expr->funId, newparams, expr->refParams);
        }

        IrElement *Ssa::visit(Function const *const expr) {
            Function *transformed = (expr->isNative()) ?
                    new Function(expr->id, expr->nativeAddress, expr->returnType, expr->name):
                    new Function(expr->id, expr->returnType, NULL, expr->name);

            for (auto p : expr->parametersIds)
                transformed->parametersIds.push_back((shouldBeRenamed(p)) ? newName(p) : p);
            for (auto p : expr->memoryCells)
                transformed->memoryCells.push_back((shouldBeRenamed(p)) ? newName(p) : p);
            for (auto p : expr->refParameterIds)
                transformed->refParameterIds.push_back(/*(shouldBeRenamed(p)) ? newName(p) :*/ p);
            Block *newEntry = static_cast<Block *> ( expr->entry->visit(this) );
            if (newEntry == NULL) {
                delete transformed;
                return NULL;
            }
            transformed->entry = newEntry;
            return transformed;
        }

        IrElement *Ssa::visit(WriteRef const *const expr) {

            auto e = expr->atom->visit(this);
            if (!e) return NULL;
                return new WriteRef((Atom const *const) e,  expr->refId );
        }

        IrElement *Ssa::visit(ReadRef const *const expr) {
//            if (shouldBeRenamed(expr->refId))
//                return new ReadRef(_latestVersion[expr->refId]);
            return base::visit(expr);
        }


    }
}