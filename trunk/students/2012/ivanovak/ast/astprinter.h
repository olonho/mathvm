#ifndef ___astprinter____
#define ___astprinter____

#include "ast.h"

class ASTPrinter : public mathvm::AstVisitor {
public:
    
    ASTPrinter();
    virtual ~ASTPrinter();
    
    #define VISITOR_FUNCTION(type, name) \
    virtual void visit##type(mathvm::type* node);

    FOR_NODES(VISITOR_FUNCTION)
    #undef VISITOR_FUNCTION
};

#endif
