//
// Created by austud on 10/12/16.
//

#ifndef MATHVM_TRANSFORMER_H
#define MATHVM_TRANSFORMER_H

#include <sstream>

#include "../../../include/ast.h"
#include "../../../include/mathvm.h"
#include "../../../include/visitors.h"

using namespace std;

namespace mathvm
{

class transformer : public AstBaseVisitor
{
private:
    stringstream ss;
    int level;

    static inline string token(TokenKind k) {
#define CHECK_TYPE(t, s, p) if (k == t) return s;
        FOR_TOKENS(CHECK_TYPE);
        return "";
    }

    static string string_literal_transform(string l);

public:

    transformer(bool top_level = false) {
        level = !top_level;
    }

    string to_str() { return ss.str(); }

#define VISITOR_FUNCTION_OVERRIDE(type, name) \
    virtual void visit##type(type* node) override;

    FOR_NODES(VISITOR_FUNCTION_OVERRIDE)
#undef VISITOR_FUNCTION_OVERRIDE

};

} // mathvm


#endif //MATHVM_TRANSFORMER_H
