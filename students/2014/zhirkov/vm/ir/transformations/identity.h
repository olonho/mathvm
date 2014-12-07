#pragma once
#include "../ir.h"
#include <map>


namespace mathvm {
    namespace IR {

        struct IdentityTransformation  : public IrVisitor {
            FOR_IR(VISITOR)

        private:
            std::map<IrElement const*, IrElement*> _visited;
            bool visited(IrElement* e) {return _visited.find(e) != _visited.end(); }
        };

    }
}