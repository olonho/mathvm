#ifndef AST_PRINTER_H
#define AST_PRINTER_H

#include "visitors.h"

namespace mathvm {

class AstPrinter : public AstVisitor {
    static constexpr int TABSTOP = 4;

public:
    explicit AstPrinter(std::ostream &os = std::cout);
    void dump(AstNode *rootNode);

#define VISITOR_FUNCTION(type, name) \
    void visit##type(type *node);

    FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION

private:
    std::ostream & out_;
    size_t level_;
    bool isOneLiner(AstNode *node);
    void increaseLevel();
    void decreaseLevel();
    void print(const std::string &expr, bool withIndent = false);
    void indent();
    string escapeString(const std::string &s);
};

}

#endif // AST_PRINTER_H

