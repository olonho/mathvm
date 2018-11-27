#ifndef typer_hpp_INCLUDED
#define typer_hpp_INCLUDED

#include <mathvm.h>
#include <ast.h>

#define INFO(node) (reinterpret_cast<unsigned long long int>((node)->info()))

#define GET_TYPE(node) ((Type)INFO((AstNode*)node))
#define GET_CTX(var) (INFO((AstVar*)var) >> 16)
#define GET_VAR_ID(var) (INFO((AstVar*)var) & 0xFFFF)
#define GET_FN_ID(fn) (((BytecodeFunction*)(((AstFunction*)fn)->info()))->id())

namespace mathvm {

enum Type {
    NONE = VT_VOID,
    INT = VT_INT,
    DOUBLE = VT_DOUBLE,
    STRING = VT_STRING,
    BOOL,
};

void assignTypes(AstFunction *top, Code *code);

}

#endif // typer_hpp_INCLUDED
