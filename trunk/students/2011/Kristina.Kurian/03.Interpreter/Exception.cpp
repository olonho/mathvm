#include <sstream>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <assert.h>

#include "Translator.h"
#include "AstShowVisitor.h"

void throwError(mathvm::AstNode* node, const char* format, ...) {
    char *buf;
    va_list args;
    va_start(args, format);
    vasprintf(&buf, format, args);
    mathvm::Status s(buf, node->position());
    free(buf);
    throw s;
}

std::string showExpr(mathvm::AstNode* node) {
    std::stringstream str;
    AstShowVisitor v(str);
    v.show(node);
    return str.str();
}

void typeMismatch(const char* e, mathvm::AstNode* expr, mathvm::VarType a) {
    throwError(expr, "Expected expression of type %s, but %s has type %s",
        e, showExpr(expr).c_str(), typeToName(a));
}
