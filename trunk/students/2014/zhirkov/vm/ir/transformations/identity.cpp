#include "identity.h"
#include "../ir.h"
#include <memory>

namespace mathvm {
    namespace IR {

        IrElement *Transformation::visit(BinOp const *const expr) {
            Atom const *left = static_cast<Atom const *> (expr->left->visit(this));
            if (left == NULL) return NULL;
            Atom const *right = static_cast<Atom const *> (expr->right->visit(this));
            if (right == NULL) {
                delete left;
                return NULL;
            }
            return new BinOp(left, right, expr->type);
        }

        IrElement *Transformation::visit(UnOp const *const expr) {
            Atom const *operand = static_cast<Atom const *> (expr->operand->visit(this));
            if (operand == NULL) return NULL;
            return new UnOp(operand, expr->type);
        }

        IrElement *Transformation::visit(Variable const *const expr) {
            return new Variable(expr->id);
        }

        IrElement *Transformation::visit(Return const *const expr) {
            Atom const *operand = static_cast<Atom const *> (expr->atom->visit(this));
            return new Return(operand);
        }

        IrElement *Transformation::visit(Phi const *const expr) {
            Variable* var = static_cast<Variable*> (expr->var->visit(this));
            if (!var) return NULL;
            auto res = new Phi(var);
            for (auto v : expr->vars) {
                Variable const *var = static_cast<Variable const *> (v->visit(this));
                if (var != NULL) res->vars.insert(var);
            }
            return res;
        }

        IrElement *Transformation::visit(Int const *const expr) {
            return new Int(expr->value);
        }

        IrElement *Transformation::visit(Double const *const expr) {
            return new Double(expr->value);
        }

        IrElement *Transformation::visit(Ptr const *const expr) {
            return new Ptr(*expr);
        }

        IrElement *Transformation::visit(Block const *const expr) {
            _currentSourceBlock = expr;
            Block *result = new Block(expr->name + "'");
            _currentResultBlock = result;
//            result->predecessors = expr->predecessors;
            for (auto st : expr->contents) {
                auto transformed = st->visit(this);
                if (transformed != NULL) result->contents.push_back(static_cast<IR::Statement *>(transformed));
            }
            _visited[expr] = result;
            if (expr->getTransition())
                result->setTransition(static_cast<Jump *>( expr->getTransition()->visit(this) ));


            return result;
        }

        IrElement *Transformation::visit(Assignment const *const expr) {
            auto lhs = expr->var->visit(this);
            if (lhs == NULL) return NULL;
            auto rhs = expr->value->visit(this);
            if (rhs == NULL) {
                delete lhs;
                return NULL;
            }
            return new Assignment(lhs->asVariable(), static_cast<Expression const *>(rhs));
        }

        IrElement *Transformation::visit(Call const *const expr) {
            std::vector<const Atom*> params;
            for (auto p : expr->params) {
                Atom const* tp = static_cast<Atom*>( p->visit(this) );
                if (tp) params.push_back(tp);
            }
            return new Call(expr->funId, params, expr->refParams);
        }

        IrElement *Transformation::visit(Print const *const expr) {
            Atom const *inner = static_cast<Atom const *>(expr->atom->visit(this));
            return new Print(inner);
        }

        IrElement *Transformation::visit(FunctionRecord const *const expr) {
            _currentSourceFunction = expr;
            Block* newEntry = static_cast<Block*> ( expr->entry->visit(this) );
            if (newEntry == NULL) return NULL;

            FunctionRecord *transformed = new FunctionRecord(expr->id, expr->returnType, newEntry);
            _currentResultFunction = transformed;
            transformed->parametersIds = expr->parametersIds;
            transformed->memoryCells = expr->memoryCells;
            transformed->refParameterIds = expr->refParameterIds;
            transformed->returnType = expr->returnType;
            return transformed;
        }

        IrElement *Transformation::visit(JumpAlways const *const expr) {
            auto in = _currentResultBlock;
            if (visited(&(*(expr->destination))))
                return new JumpAlways(const_cast<Block*>(static_cast<Block const*>(_visited[&(*(expr->destination))])));
            else {
                Block *b = const_cast<Block*>(static_cast<Block const*>(expr->destination->visit(this)));
                if (b == NULL)  return NULL;
                else {
                    b->addPredecessor(in);
                    return new JumpAlways(b);
                }
            }
        }

        IrElement *Transformation::visit(JumpCond const *const expr) {
            auto in = _currentResultBlock;
            auto condvar = static_cast<Atom*> (expr->condition->visit(this));
            if (!condvar) return NULL;
            Block* tyes = NULL, *tno = NULL;
            if (visited(&(*(expr->yes)))) tyes = (Block*)(_visited[&(*(expr->yes))]);
            else  tyes = (Block*)( expr->yes->visit(this) );
            if (visited(&(*(expr->no)))) tno = (Block*)(_visited[&(*(expr->no))]);
            else  tno = (Block*)( expr->no->visit(this) );


            if (tyes && tno) {
                tno->addPredecessor(in);
                tyes -> addPredecessor(in);
                return new JumpCond(tyes, tno, condvar );
            }
            delete tyes; delete tno;
            return NULL;
        }

        IrElement *Transformation::visit(WriteRef const *const expr) {
            auto trAtom = expr->atom->visit(this);
            if (!trAtom) return NULL;
            return new WriteRef((Atom const *const) trAtom, expr->refId);
        }

        IrElement *Transformation::visit(ReadRef const *const expr) {
            return new ReadRef(expr->refId);
        }


        Transformation copier(NULL, "identity", std::cerr);

        void Transformation::visit(SimpleIr::StringPool const &pool) {
            for(auto s : pool)
                _currentIr->pool.push_back(s);
        }
    }
}