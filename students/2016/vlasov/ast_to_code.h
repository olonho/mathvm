//
// Created by svloyso on 17.10.16.
//

#ifndef MATHVM_AST_TO_CODE_H
#define MATHVM_AST_TO_CODE_H

#include <iostream>
#include <string>

#include "ast.h"

namespace mathvm {

class AstToCode : protected AstVisitor {
    std::ostream& result;
    int _indent;
    const int tab = 4;
public:
    AstToCode(std::ostream& out) : result(out), _indent(0) {}
    void dumpCode(AstNode* root);

protected:
    const char* lpar()  { return "("; }
    const char* rpar()  { return ")"; }
    const char* space() { return " "; }
    const char* line()  { return "\n"; }
    const char* lbr()   { return "{"; }
    const char* rbr()   { return "}"; }
    const char* semi()  { return ";"; }
    const char* com()   { return ","; }
    const char* quot()  { return "'"; }
    std::string indent(){ return std::string(_indent, ' '); };

    void incrIndent() { _indent += tab; }
    void decrIndent() { _indent -= tab; }

    void dumpVar(const AstVar* var);
    void dumpBlock(BlockNode* block, bool mandatoryBraces = true);

#define VISITOR_FUNCTION(type, name) \
    void visit##type(type* node);

        FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION
};

}

#endif //MATHVM_AST_TO_CODE_H
