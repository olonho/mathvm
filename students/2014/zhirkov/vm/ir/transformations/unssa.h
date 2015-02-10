#pragma once

#include <map>
#include <set>
#include <queue>
#include "identity.h"

namespace mathvm {
    namespace IR {
        struct UnSSA : public Transformation<> {
            virtual ~UnSSA() {
            }


            UnSSA(SimpleIr const& source, SimpleIr& dest, std::ostream &debug = std::cerr)
                    : Transformation(source, dest, "phi unwinder", debug) {
            }

            virtual IrElement *visit(Phi const *const expr) {
                phiBlocks.push_back(make_pair(_currentResultBlock, expr));
                return base::visit(expr);
            }


            virtual void operator()() {
                Transformation::visit(&_oldIr);

                for (auto interestingBlockPair : phiBlocks) {
                    Block *interestingBlock = interestingBlockPair.first;
                    Phi const *st = interestingBlockPair.second;

                    std::queue<Block *> queue;

                    for (auto pred : interestingBlock->predecessors) queue.push((Block *) pred);
                    while (!queue.empty()) {
                        auto block = queue.front();
                        queue.pop();
                        auto success = fillBlock(block, st->asPhi());
                        if (!success) for (auto pred : block->predecessors) queue.push((Block *) pred);
                    }
                }
            }

            static bool fillBlock(Block *b, Phi const *const phi) {
                for (auto v : phi->vars) {
                    auto varId = v->id;
                    if (declaresVar(b, varId)) {
                        b->contents.push_back(new Assignment(phi->var->id, new Variable(varId)));
                        return true;
                    }
                }
                return false;
            }

            static bool declaresVar(Block const *block, VarId id) {
                for (auto st : block->contents)
                    if (st->isAssignment()) if (st->asAssignment()->var->id == id)
                        return true;
                return false;
            }

        private:
            std::vector<std::pair<Block *, Phi const *>> phiBlocks;

        };

    }
}
