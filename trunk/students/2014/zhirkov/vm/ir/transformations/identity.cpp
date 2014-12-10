#include "identity.h"
#include "../ir.h"
#include <memory>

namespace mathvm {
    namespace IR {

        IrElement *IdentityTransformation::visit(BinOp const *const expr) {
            Expression const *left = static_cast<Expression const *> (expr->left->visit(this));
            if (left == NULL) return NULL;
            Expression const *right = static_cast<Expression const *> (expr->right->visit(this));
            if (right == NULL) {
                delete left;
                return NULL;
            }
            return new BinOp(left, right, expr->type);
        }

        IrElement *IdentityTransformation::visit(UnOp const *const expr) {
            Expression const *operand = static_cast<Expression const *> (expr->operand->visit(this));
            if (operand == NULL) return NULL;
            return new UnOp(operand, expr->type);
        }

        IrElement *IdentityTransformation::visit(Variable const *const expr) {
            return new Variable(expr->id);
        }

        IrElement *IdentityTransformation::visit(Return const *const expr) {
            Atom const *operand = static_cast<Atom const *> (expr->atom->visit(this));
            return new Return(operand);
        }

        IrElement *IdentityTransformation::visit(Phi const *const expr) {
            Variable* var = static_cast<Variable*> (expr->var->visit(this));
            if (!var) return NULL;
            auto res = new Phi(var);
            for (auto v : expr->vars) {
                Variable const *var = static_cast<Variable const *> (v->visit(this));
                if (var != NULL) res->vars.insert(var);
            }
            return res;
        }

        IrElement *IdentityTransformation::visit(Int const *const expr) {
            return new Int(expr->value);
        }

        IrElement *IdentityTransformation::visit(Double const *const expr) {
            return new Double(expr->value);
        }

        IrElement *IdentityTransformation::visit(Ptr const *const expr) {
            return new Ptr(*expr);
        }

        IrElement *IdentityTransformation::visit(Block const *const expr) {
            _currentSourceBlock = expr;
            Block *result = new Block(expr->name);
            result->predecessors = expr->predecessors;
            for (auto st : expr->contents) {
                auto transformed = st->visit(this);
                if (transformed != NULL) result->contents.push_back(static_cast<IR::Statement *>(transformed));
            }
            _visited[expr] = result;
            if (expr->getTransition())
                result->setTransition(static_cast<Jump *>( expr->getTransition()->visit(this) ));


            return result;
        }

        IrElement *IdentityTransformation::visit(Assignment const *const expr) {
            auto lhs = expr->var->visit(this);
            if (lhs == NULL) return NULL;
            auto rhs = expr->value->visit(this);
            if (rhs == NULL) {
                delete lhs;
                return NULL;
            }
            return new Assignment(lhs->asVariable()->id, static_cast<Expression const *>(rhs));
        }

        IrElement *IdentityTransformation::visit(Call const *const expr) {
            std::vector<const Atom*> params;
            for (auto p : expr->params) {
                Atom const* tp = static_cast<Atom*>( p->visit(this) );
                if (tp) params.push_back(tp);
            }
            return new Call(expr->funId, params);
        }

        IrElement *IdentityTransformation::visit(Print const *const expr) {
            Atom const *inner = static_cast<Atom const *>(expr->atom->visit(this));
            return new Print(inner);
        }

        IrElement *IdentityTransformation::visit(FunctionRecord const *const expr) {
            Block* newEntry = static_cast<Block*> ( expr->entry->visit(this) );
            if (newEntry == NULL) return NULL;

            FunctionRecord *transformed = new FunctionRecord(expr->id, expr->returnType, newEntry);
            if (transformed == NULL) { delete newEntry; return NULL; }
            transformed->parametersIds = expr->parametersIds;
            return transformed;
        }

        IrElement *IdentityTransformation::visit(JumpAlways const *const expr) {
            if (visited(&(*(expr->destination))))
                return new JumpAlways(static_cast<Block*>(_visited[&(*(expr->destination))]));
            else {
                Block *b = static_cast<Block *>(expr->destination->visit(this));
                if (b == NULL)  return NULL;
                else return new JumpAlways(b);
            }
        }

        IrElement *IdentityTransformation::visit(JumpCond const *const expr) {
            auto condvar = static_cast<Atom*> (expr->condition->visit(this));
            if (!condvar) return NULL;
            Block* tyes = NULL, *tno = NULL;
            if (visited(&(*(expr->yes)))) tyes = static_cast<Block*>(_visited[&(*(expr->yes))]);
            else  tyes = static_cast<Block*>( expr->yes->visit(this) );
            if (visited(&(*(expr->no)))) tno = static_cast<Block*>(_visited[&(*(expr->no))]);
            else  tno = static_cast<Block*>( expr->no->visit(this) );

            if (tyes && tno) return new JumpCond(tyes, tno, condvar );
            delete tyes; delete tno;
            return NULL;
        }

        IrElement *IdentityTransformation::visit(WriteRef const *const expr) {
            auto trAtom = expr->atom->visit(this);
            if (!trAtom) return NULL;
            return new WriteRef((Atom const *const) trAtom, expr->refId);
        }

        IrElement *IdentityTransformation::visit(ReadRef const *const expr) {
            return new ReadRef(expr->refId);
        }
    }
}