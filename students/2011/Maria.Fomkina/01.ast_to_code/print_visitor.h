#ifndef _PRINT_VISITOR_H
#define _PRINT_VISITOR_H

#include "mathvm.h"
#include "ast.h"

namespace mathvm {

class PrintVisitor : public AstVisitor {
  public:
    PrintVisitor() {
    }
    virtual ~PrintVisitor() {
    }

#define VISITOR_FUNCTION(type, name) \
    virtual void visit##type(type* node);

    FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION
};

}

#endif /* _PRINT_VISITOR_H */
