#ifndef AST_PRINT_VISITOR
#define AST_PRINT_VISITOR

#include "visitors.h"

namespace mathvm {


class AstPrintVisitor : public AstBaseVisitor {

public:
    AstPrintVisitor() {}

#define VISITOR_FUNCTION(type, name)\
    virtual void visit##type(type* node);

    FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION


    virtual ~AstPrintVisitor() {}

private:
    void visitBlockNodeHelper(BlockNode * node, bool needsBraces);
    std::string makeRaw(const std::string& str);
};

} //namespace
#endif
