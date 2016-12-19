#ifndef OPERATIONS_H
#define OPERATIONS_H

#include "ast.h"
#include "mathvm.h"
#include "generator_exception.h"

namespace mathvm {

bool isLogicalOp(TokenKind token) {
    return token == tOR || token == tAND;
}

bool isBitwiseOp(TokenKind token) {
    switch (token) {
        case tAOR:
        case tAAND:
        case tAXOR:
            return true;
        default:
            return false;
    }
}

Instruction bitInsn(TokenKind token) {
    switch (token) {
        case tAOR:
            return BC_IAOR;
        case tAAND:
            return BC_IAAND;
        case tAXOR:
            return BC_IAXOR;
        default:
            throw new BytecodeGeneratorException("Invalid logical instruction");
    }
}

Instruction printInsn(VarType type) {
    switch (type) {
        case VT_INT:
            return BC_IPRINT;
        case VT_DOUBLE:
            return BC_DPRINT;
        case VT_STRING:
            return BC_SPRINT;
        default:
            throw new BytecodeGeneratorException("Invalid print operation");
    }
}

bool isCompareOp(TokenKind token) {
    switch (token) {
        case tEQ:
        case tNEQ:
        case tGE:
        case tLE:
        case tGT:
        case tLT:
            return true;
        default:
            return false;
    }
}

Instruction compareInsn(TokenKind token) {
    switch (token) {
        case tEQ:
            return BC_IFICMPE;
        case tNEQ:
            return BC_IFICMPNE;
        case tGE:
            return BC_IFICMPGE;
        case tLE:
            return BC_IFICMPLE;
        case tGT:
            return BC_IFICMPG;
        case tLT:
            return BC_IFICMPL;
        default:
            throw BytecodeGeneratorException("Invalid compare operation");
    }
}

bool isArithmeticOp(TokenKind token) {
    switch (token) {
        case tADD:
        case tSUB:
        case tMUL:
        case tDIV:
        case tMOD:
            return true;
        default:
            return false;
    }
}

Instruction arithmeticInsn(TokenKind token, VarType type) {
    switch (token) {
        case tADD:
            return type == VT_INT ? BC_IADD : BC_DADD;
        case tSUB:
            return type == VT_INT ? BC_ISUB : BC_DSUB;
        case tMUL:
            return type == VT_INT ? BC_IMUL : BC_DMUL;
        case tDIV:
            return type == VT_INT ? BC_IDIV : BC_DDIV;
        case tMOD:
            return BC_IMOD;
        default:
            throw new BytecodeGeneratorException("Invalid arithmetic instruction");
    }
}

Instruction localStoreInsn(VarType type) {
    switch (type) {
        case VT_INT:
            return BC_STOREIVAR;
        case VT_DOUBLE:
            return BC_STOREDVAR;
        case VT_STRING:
            return BC_STORESVAR;
        default:
            throw BytecodeGeneratorException("Wrong variable type");
    }
}

Instruction contextStoreInsn(VarType type) {
    switch (type) {
        case VT_INT:
            return BC_STORECTXIVAR;
        case VT_DOUBLE:
            return BC_STORECTXDVAR;
        case VT_STRING:
            return BC_STORECTXSVAR;
        default:
            throw BytecodeGeneratorException("Wrong variable type");
    }
}

Instruction localLoadInsn(VarType type) {
    switch (type) {
        case VT_INT:
            return BC_LOADIVAR;
        case VT_DOUBLE:
            return BC_LOADDVAR;
        case VT_STRING:
            return BC_LOADSVAR;
        default:
            throw BytecodeGeneratorException("Wrong variable type");
    }
}

Instruction contextLoadInsn(VarType type) {
    switch (type) {
        case VT_INT:
            return BC_LOADCTXIVAR;
        case VT_DOUBLE:
            return BC_LOADCTXDVAR;
        case VT_STRING:
            return BC_LOADCTXSVAR;
        default:
            throw BytecodeGeneratorException("Wrong variable type");
    }
}

}

#endif //OPERATIONS_H
