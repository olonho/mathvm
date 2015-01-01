#include <iostream>
#include <queue>
#include "phi_values.h"
#include "../ir_printer.h"

namespace mathvm {
    namespace IR {

        IrElement *PhiFiller::visit(Phi const *const expr) {
            if (_currentIr->varMeta[expr->var->id].isSourceVar) {
                auto oldVarId = _currentIr->varMeta[expr->var->id].originId;

                _debug << "Calculating phi function for variable " << expr->var->id << " (source id " << oldVarId << ")\n";
                Phi * result = new Phi(expr->var->id);
                std::set<VarId> varids;


                std::queue<Block const*> blocks;
                blocks.push(_currentSourceBlock);

                IrPrinter printer(_debug);
                while(!blocks.empty()) {
                    Block const* const b = blocks.front();
                    blocks.pop();
                    for( auto it = b->contents.rbegin(); it != b->contents.rend(); ++it)
                        if ((*it)->isAssignment()){
                            _debug << "candidate: " ;
                            (*it)->visit(&printer);
                            _debug<<"\n";
                            auto lhs = (*it)->asAssignment()->var->id;
                            auto& meta = _currentIr->varMeta[lhs];
                            if (meta.isSourceVar && meta.originId == oldVarId) {
                                _debug << "phi function for " << expr->var->id << " will get an argument " << lhs << std::endl;
                                varids.insert(lhs);
                                goto next;
                            }
                        }
                    for (Block const* p : b->predecessors)
                        blocks.push(p);
                    next: {};
                }

                for (auto id : varids)
                    result->vars.insert(new Variable(id));
                return result;
            }
            return Transformation::visit(expr);
        }
    }
}