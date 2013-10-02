#ifndef _MATHVM_AST_PRINTER_H
#define _MATHVM_AST_PRINTER_H

#include "visitors.h"

namespace mathvm {

  class AstPrinter : public AstBaseVisitor {
  public:
    AstPrinter() {}
    virtual ~AstPrinter() {}

#define VISITOR_FUNCTION(type, name) \
    virtual void visit##type(type * node); 
 
    FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION
		
  };

}
#endif
