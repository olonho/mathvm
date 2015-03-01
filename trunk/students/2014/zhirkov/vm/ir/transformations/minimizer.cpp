#include "minimizer.h"
#include "../../translator/ssa_utils.h"

namespace mathvm {
    namespace IR {

        void Minimizer::replaceSucc(Block * parent, Block * old) {
            Block *subst = (Block *) old->getTransition()->asJumpAlways()->destination;
            subst->removePredecessor(old);

            if (parent->getTransition()->isJumpAlways()) {
                JumpAlways const *jmp = parent->getTransition()->asJumpAlways();
                assert(jmp->destination == old);
                parent->link(subst);
                subst->addPredecessor(parent);
            }
            else {
                JumpCond const *jmp = parent->getTransition()->asJumpCond();
                if (jmp->yes == old) parent->setTransition(jmp->replaceYes(subst));
                if (jmp->no == old) parent->setTransition(jmp->replaceNo(subst));
                subst->addPredecessor(parent);
            }
        }

        IrElement *Minimizer::visit(const Function *const expr) {
            Function *fr = (Function *) base::visit(expr);
            auto blocks = blocksOrder(fr->entry);
            for (auto b : blocks) {
                _debug << "Processing " << b->name << std::endl;
                if (b->isEmpty() && !b->isEntry()) {
                    _debug << "removing empty block " << b->name << std::endl;
                    auto preds = b->predecessors;
                    for (auto p : preds)
                        replaceSucc((Block *) p, (Block *) b);

                }
                else _debug << "block " << b->name << " is not empty" << std::endl;
            }
            return fr;
        }

        void Minimizer::operator()() {
            Transformation::visit(&_oldIr);
        }
    }

}