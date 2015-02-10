#include "ir.h"
#include "../translator/ssa_utils.h"
#include "transformations/identity.h"


namespace mathvm {
    namespace IR {

        FunctionRecord::~FunctionRecord() {
            if (entry)
                for( auto b : blocksPostOrder(entry))
                    delete b;
        }

        JumpCond* JumpCond::replaceYes(Block *const repl) const { return new JumpCond( repl, no,  (Atom const*) condition->visit(&copier)); }
        JumpCond* JumpCond::replaceNo(Block *const repl) const { return new JumpCond( yes, repl,  (Atom const*) condition->visit(&copier)); }

    }
}