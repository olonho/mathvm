#include "typeinferencer.h"

namespace mathvm
{

const TokenKind TypeInferencer::assignOps[]  = {tASSIGN, tINCRSET, tDECRSET};
const TokenKind TypeInferencer::compareOps[] = {tEQ, tNEQ, tNOT, tLT, tLE, tGT, tGE, tOR, tAND};
const TokenKind TypeInferencer::bitOps[]     = {tAOR, tAAND, tAXOR};
const TokenKind TypeInferencer::ariphmOps[]  = {tADD, tSUB, tMUL, tDIV};


bool TypeInferencer::find(TokenKind op, const TokenKind ops[], int count) {
    for (int i = 0; i < count; ++i) {
        if (ops[i] == op) {
            return true;
        }
    }
    return false;
}

VarType TypeInferencer::commonTypeForBinOp(TokenKind binOp, VarType left, VarType right)
{
    if (find(binOp, assignOps, ASSIGN_COUNT)) {
        return left;
    }

    else if (find(binOp, compareOps, COMPARE_COUNT)) {
        return commonType(left, right);
    }

    else if (find(binOp, bitOps, BIT_COUNT)) {
        return VT_INT;
    }

    else if (find(binOp, ariphmOps, ARIPHM_COUNT)) {
        return commonType(left, right);
    }

    else if (binOp == tMOD) {
        return VT_INT;
    }

    return VT_INVALID;
}

VarType TypeInferencer::commonType(const VarType type1, const VarType type2) {
    if (type1 == VT_DOUBLE || type2 == VT_DOUBLE) {
        return VT_DOUBLE;
    }
    else if (type1 == VT_INT || type2 == VT_INT) {
        return VT_INT;
    }

    return VT_STRING;
}

}
