#ifndef TYPEINFERENCER_H
#define TYPEINFERENCER_H

#include "ast.h"
#include "mathvm.h"

namespace mathvm {

class TypeInferencer : public AstVisitor
{
    static const int ASSIGN_COUNT = 3;
    static const int COMPARE_COUNT = 9;
    static const int BIT_COUNT = 3;
    static const int ARIPHM_COUNT = 4;

    static const TokenKind assignOps[];
    static const TokenKind compareOps[];
    static const TokenKind bitOps[];
    static const TokenKind ariphmOps[];

    static VarType commonTypeForBinOp(TokenKind binOp, VarType left, VarType right);

    static bool find(TokenKind op, const TokenKind ops[], int count);

public:
    static VarType commonType(VarType v1, VarType v2);
};

}

#endif // TYPEINFERENCER_H
