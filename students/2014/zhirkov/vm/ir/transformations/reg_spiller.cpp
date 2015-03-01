#include "reg_spiller.h"

namespace mathvm {
    namespace IR {

        bool RegSpiller::isStackAllocated(uint64_t id) const {
            auto const &alloc = allocations.at(_currentSourceFunction->id).stackAlloc;
            return alloc.find(id) != alloc.end();
        }

        IrElement *RegSpiller::visit(const Assignment *const expr) {
            if (isStackAllocated(expr->var->id)) {
                VarId accReg = (_oldIr.varMeta[expr->var->id].type == VT_Double)? doubleAcc : gpAcc;
                _currentResultBlock->contents.push_back(new Assignment(accReg, (Expression const *) expr->value->visit(this)));
                if (!hasMemoryCell(expr->var->id))
                    _currentResultFunction->memoryCells.push_back(expr->var->id);
                return new WriteRef(new Variable(accReg), expr->var->id);
            }
            return base::visit(expr);
        }

        IrElement *RegSpiller::visit(const Variable *const expr) {
            if (isStackAllocated(expr->id)) return new ReadRef(expr->id);
            return base::visit(expr);
        }

        IrElement *RegSpiller::visit(const Function *const expr) {
            Function *visited = (Function *) base::visit(expr);
            for (auto p : expr->parametersIds)
                if (isStackAllocated(expr->id)) {
                    visited->memoryCells.push_back(p);
                    visited->entry->contents.push_front(new WriteRef(new Variable(p), p));
                }
            return visited;
        }
    }
}