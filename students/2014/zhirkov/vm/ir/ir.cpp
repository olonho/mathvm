#include "ir.h"
#include "../translator/ssa_utils.h"


namespace mathvm {
    namespace IR {

        FunctionRecord::~FunctionRecord() {
            if (entry)
                for( auto b : blocksPostOrder(entry))
                    delete b;
        }
    }
}