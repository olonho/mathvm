#include <queue>
#include <algorithm>
#include "ssa_utils.h"

namespace mathvm {


    static void blocksPostOrder(const IR::Block *const block, std::vector<const IR::Block *> &acc, std::set<const IR::Block *> &visited) {
        if (visited.find(block) != visited.end()) return;
        visited.insert(block);
        if (block->getTransition() != NULL) {
            if (block->getTransition()->isJumpAlways())
                blocksPostOrder(&(*(block->getTransition()->asJumpAlways()->destination)), acc, visited);
            else {
                blocksPostOrder(block->getTransition()->asJumpCond()->yes, acc, visited);
                blocksPostOrder(block->getTransition()->asJumpCond()->no, acc, visited);
            }
        }
        acc.push_back(block);
    }

//    std::vector<const IR::Block *> allBlocks(const IR::Block *const startBlock) {
//        std::vector<const IR::Block *> blocks;
//        std::queue<const IR::Block *> q;
//        q.push(startBlock);
//        while (!q.empty()) {
//            const IR::Block *current = q.front();
//            q.pop();
//            if (blocks.find(current) == blocks.end()) {
//                blocks.push_back(current);
//                IR::Jump *trans = current->getTransition();
//                if (trans->isJumpAlways()) q.push(trans->asJumpAlways()->destination);
//                else if (trans->isJumpCond()) {
//                    q.push(trans->asJumpCond()->yes);
//                    q.push(trans->asJumpCond()->no);
//                }
//            }
//        }
//        return blocks;
//    }

    std::vector<const IR::Block *> blocksPostOrder(const IR::Block *const startBlock) {
        std::vector<const IR::Block *> res;
        std::set<const IR::Block *> visited = std::set<const IR::Block *>();
        blocksPostOrder(startBlock, res, visited);
        return res;
    }

    std::set<const IR::Block *> collectPredecessors(IR::Block *block) {
        std::set<const IR::Block *> preds;
        std::queue<const IR::Block *> q;
        q.push(block);
        while (!q.empty()) {
            const IR::Block *current = q.front();
            q.pop();
            if (preds.find(current) == preds.end()) {
                preds.insert(current);
                for (auto it = current->predecessors.begin(); it != current->predecessors.end(); ++it)
                    q.push(*it);
            }
        }
        preds.erase(block);
        return preds;
    }


    std::set<const IR::Block *> intersect(std::set<const IR::Block *> const &s, std::set<const IR::Block *> const &operand) {
        std::set<const IR::Block *> result;
        for (auto it = s.begin(); it != s.end(); it++) if (operand.find(*it) != operand.end()) result.insert(*it);
        return result;
    }


    Dominators dominators(const IR::Block *const startBlock) {

        std::vector<const IR::Block *> blocks = blocksPostOrder(startBlock);
        std::set<const IR::Block *> all(blocks.begin(), blocks.end());

        std::map<const IR::Block *, std::set<const IR::Block *>> dom;

        for (std::vector<const IR::Block *>::iterator it = blocks.begin(); it != blocks.end(); ++it)
            dom[*it] = all;
        dom[startBlock] = std::set<const IR::Block *>();
        dom[startBlock].insert(startBlock);

        bool changed = true;

        while (changed) {
            changed = false;

            for (auto it = blocks.rbegin(); it != blocks.rend(); ++it) {
                if (*it == startBlock) continue;
                IR::Block const &node = **it;
                std::set<const IR::Block *> intersection(all.begin(), all.end());
                for (std::vector<const IR::Block *>::const_iterator p = node.predecessors.cbegin(); p != node.predecessors.cend(); ++p)
                    intersection = intersect(intersection, dom[*p]);
                intersection.insert(&node);

                if (intersection != dom[&node]) {
                    changed = true;
                    dom[&node] = intersection;
                }
            }
        }
//        for (auto it = dom.begin(); it != dom.end(); ++it)
//            (*it).second.erase((*it).first);

        return dom;
    }

    template<class T>
    static bool contains(std::set<T>& s, T& elem) {
        return s.find(elem) != s.end();
    }

    static bool isIDom(const IR::Block * candidate, const IR::Block * forBlock, Dominators & doms ) {
        for (auto dominator : doms[forBlock]) if (dominator != forBlock) {
            if (dominator == forBlock || dominator == candidate) continue;

            if (contains(doms[dominator], candidate)) return false;
        }
        return true;
    }
    std::map<const IR::Block *, const IR::Block *> immediateDominators(const IR::Block *const startBlock) {
        Dominators doms = dominators(startBlock);
        std::map<const IR::Block *, const IR::Block *> idoms;
        for (auto kvp : doms)
            for (auto candidate : kvp.second) if (candidate != kvp.first)
                if (isIDom(candidate, kvp.first, doms))
                    idoms[kvp.first]= candidate;

        return idoms;
    }


    std::map<const IR::Block *, std::set<const IR::Block *>> dominanceFrontier(const IR::Block *const startBlock) {
        auto doms = immediateDominators(startBlock);
        std::map<const IR::Block *, std::set<const IR::Block *>> frontier;
        std::vector<const IR::Block *> nodes = blocksPostOrder(startBlock);

        for (std::vector<const IR::Block *>::iterator it = nodes.begin(); it != nodes.end(); ++it)
            frontier[*it] = std::set<const IR::Block *>();

        for (auto pnode : nodes)
            if (pnode->predecessors.size() >= 2) {
                for (auto pred : pnode->predecessors) {
                    auto runner = pred;
                    while (runner != doms[pnode]) {
                        frontier[runner].insert(pnode);
                        runner = doms[runner];
                    }
                }
            }
        return frontier;
    }

}