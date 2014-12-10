#include "typechecking.h"

#include "mathvm.h"
#include "ast.h"

using namespace mathvm;

//public:
bool TypeChecker::check(AstFunction* astFun) {
    _status.setOk();
    try {
        _funs.push(astFun);
        visitBlockBody(astFun->node()->body());
        _funs.pop();
        assert(_funs.size() == 0);
    } catch (ExceptionWithPos const& e) {
        _status.setError(e.what(), e.source());
        return false;
    }
    return true;
}

void TypeChecker::visitFunctionNode(FunctionNode* node) {
    if(node->body()->nodeAt(0)->isNativeCallNode()) {
        node->body()->nodeAt(0)->asNativeCallNode()->visit(this);
    } else {
        node->body()->visit(this);
    }
}

void TypeChecker::visitBlockNode(BlockNode* node) {
    visitBlockBody(node);
}

void TypeChecker::visitReturnNode(ReturnNode* node) {
    assert(_funs.size() != 0);
    VarType expectedRetType = _funs.top()->returnType();
    AstNode* retExpr = node->returnExpr();
    if(retExpr != 0) {
        retExpr->visit(this);
        VarType actualRetType = nodeType(retExpr);
        if(actualRetType != expectedRetType &&
           !isIntDoublePair(actualRetType, expectedRetType))
        {
            throw ExceptionWithPos(invalidRetExprTypeMsg(expectedRetType, actualRetType),
                                   node->position());
        }
    } else if(expectedRetType != VT_VOID) {
        throw ExceptionWithPos(retExprExpectedMsg(), node->position());
    }
}

void TypeChecker::visitNativeCallNode(NativeCallNode* node) {
    VarType retType = (node->nativeSignature()[0]).first;
    node->setInfo(new VarType(retType));
}

void TypeChecker::visitCallNode(CallNode* node) {
    size_t nodePos = node->position();
    string const callName = node->name();
    Scope* curScope = currentScope();
    AstFunction* funToCall = curScope->lookupFunction(callName);
    if(funToCall == 0) {
        throw ExceptionWithPos(undeclaredFunMsg(callName), nodePos);
    }
    for(size_t i = 0; i != node->parametersNumber(); ++i) {
        node->parameterAt(i)->visit(this);
        VarType actualArgType = nodeType(node->parameterAt(i));
        VarType expectedArgType = funToCall->parameterType(i);
        if(actualArgType != expectedArgType &&
           !isIntDoublePair(actualArgType, expectedArgType))
        {
            throw ExceptionWithPos(wrongArgTypeMsg(callName, i), nodePos);
        }
    }
    node->setInfo(new VarType(funToCall->returnType()));
}

void TypeChecker::visitPrintNode(PrintNode* node) {
    for(size_t i = 0; i != node->operands(); ++i) {
        node->operandAt(i)->visit(this);
        VarType argType = nodeType(node->operandAt(i));
        if(argType == VT_VOID || argType == VT_INVALID) {
            throw ExceptionWithPos(wrongArgTypeMsg("print", i), node->position());
        }
    }
    node->setInfo(new VarType(VT_VOID));
}

void TypeChecker::visitBinaryOpNode(BinaryOpNode* node) {
    node->left()->visit(this);
    node->right()->visit(this);
    TypeInfo* leftOp = nodeTypeInfo(node->left());
    TypeInfo* rightOp = nodeTypeInfo(node->right());
    TokenKind binOp = node->kind();
    uint32_t nodePos = node->position();
    checkTypesAreCompatible(leftOp, rightOp, nodePos);
    checkOperandsAreCorrect(leftOp, rightOp, binOp, nodePos);
    setBinOpNodeType(node, leftOp, rightOp, binOp);
}

void TypeChecker::visitUnaryOpNode(UnaryOpNode* node) {
    node->operand()->visit(this);
    uint32_t nodePos = node->position();
    VarType operandType = nodeType(node->operand());
    if(operandType != VT_INT && operandType != VT_DOUBLE) {
        throw ExceptionWithPos(unaryOpWrongTypeMsg(operandType), nodePos);
    }
    if(operandType == VT_DOUBLE && node->kind() == tNOT) {
        throw ExceptionWithPos(notOpOnDoubleMsg(), nodePos);
    }
    node->setInfo(new TypeInfo(operandType));
}

void TypeChecker::visitStringLiteralNode(StringLiteralNode* node) {
    node->setInfo(new TypeInfo(VT_STRING));
}

void TypeChecker::visitIntLiteralNode(IntLiteralNode* node) {
    node->setInfo(new TypeInfo(VT_INT));
}

void TypeChecker::visitDoubleLiteralNode(DoubleLiteralNode* node) {
    node->setInfo(new TypeInfo(VT_DOUBLE));
}

void TypeChecker::visitLoadNode(LoadNode* node) {
    VarType loadType;
    VarType varType = node->var()->type();
    switch (varType) {
    case VT_INT: loadType = VT_INT; break;
    case VT_DOUBLE: loadType = VT_DOUBLE; break;
    case VT_STRING: loadType = VT_STRING; break;
    default:
        throw ExceptionWithPos(loadNodeInvalidTypeMsg(varType), node->position());
    }
    node->setInfo(new TypeInfo(loadType));
}

void TypeChecker::visitStoreNode(StoreNode* node) {
    VarType varType = node->var()->type();
    node->value()->visit(this);
    VarType valueType = nodeType(node->value());
    if(varType == VT_INVALID ||
       varType == VT_VOID ||
       valueType == VT_VOID ||
       valueType == VT_INVALID ||
       (varType != VT_STRING && valueType == VT_STRING) ||
       (varType == VT_STRING && valueType != VT_STRING))
    {
        throw ExceptionWithPos(storeNodeInvalidOpTypesMsg(), node->position());
    }
    TokenKind assignOp = node->op();
    if(varType == VT_STRING && (assignOp == tINCRSET || assignOp == tDECRSET)) {
        throw ExceptionWithPos(compoundAssignOpOnStrMsg(), node->position());
    }
}

void TypeChecker::visitForNode(ForNode* node) {
    node->inExpr()->visit(this);
    if(node->var()->type() != VT_INT) {
        throw ExceptionWithPos(iterVarWrongTypeMsg(), node->position());
    }
    node->body()->visit(this);
}

void TypeChecker::visitWhileNode(WhileNode* node) {
    node->whileExpr()->visit(this);
    if(nodeType(node->whileExpr()) != VT_INT) {
        throw ExceptionWithPos(condExprWrongTypeMsg(), node->position());
    }
    node->loopBlock()->visit(this);
}

void TypeChecker::visitIfNode(IfNode* node) {
    node->ifExpr()->visit(this);
    if(nodeType(node->ifExpr()) != VT_INT) {
        throw ExceptionWithPos(condExprWrongTypeMsg(), node->position());
    }
    node->thenBlock()->visit(this);
    if(node->elseBlock() != 0) {
        node->elseBlock()->visit(this);
    }
}


// private:
// check impl
void TypeChecker::visitBlockBody(BlockNode* node) {
    visitFunDefs(node);
    visitExprs(node);
}

void TypeChecker::visitFunDefs(BlockNode* node) {
    Scope::FunctionIterator funIt(node->scope());
    while(funIt.hasNext()) {
        AstFunction* fun = funIt.next();
        size_t dbg_cur_size = _funs.size();
        _funs.push(fun);
        fun->node()->visit(this);
        _funs.pop();
        assert(dbg_cur_size == _funs.size());
    }
}

void TypeChecker::visitExprs(BlockNode* node) {
    for(size_t i = 0; i != node->nodes(); ++i) {
        node->nodeAt(i)->visit(this);
    }
}

// visitBinaryOpNode impl
void TypeChecker::checkTypesAreCompatible(TypeInfo* leftOp, TypeInfo* rightOp, size_t errPos) {
    string errorMsg;
    if(leftOp->type == VT_VOID || rightOp->type == VT_VOID) {
        errorMsg = binOpWrongTypeMsg(VT_VOID);
    }
    if(leftOp->type == VT_INVALID || rightOp->type == VT_INVALID) {
        errorMsg = binOpWrongTypeMsg(VT_INVALID);
    }
    if(leftOp->type == VT_STRING || rightOp->type == VT_STRING) {
        errorMsg = binOpWrongTypeMsg(VT_STRING);
    }
//    if((leftOp->type == VT_STRING && rightOp->type != VT_STRING) ||
//       (leftOp->type != VT_STRING && rightOp->type == VT_STRING))
//    {
//        errorMsg = binOpStringAndNonStringMsg();
//    }
    if(!errorMsg.empty()) {
        throw ExceptionWithPos(errorMsg, errPos);
    }
}

void TypeChecker::checkOperandsAreCorrect(TypeInfo* leftOp, TypeInfo* rightOp, TokenKind binOp, size_t errPos) {
    string errorMsg;
//    if(leftOp->type == VT_STRING && binOp != tEQ && binOp != tNEQ) {
//        errorMsg = invalidBinOpOnStrMsg(binOp);
//    }
    if((leftOp->type == VT_DOUBLE || rightOp->type == VT_DOUBLE) &&
        isIntsExpectingOp(binOp))
    {
        errorMsg = invalidBinOpOnDoubleMsg(binOp);
    }
    if(!errorMsg.empty()) {
        throw ExceptionWithPos(errorMsg, errPos);
    }
}

void TypeChecker::setBinOpNodeType(AstNode* node, TypeInfo* leftOp, TypeInfo* rightOp, TokenKind binOp) {
    VarType nodeType = VT_DOUBLE;
    if(binOp == tRANGE) {
        nodeType = VT_INVALID;
    } else if (leftOp->type == rightOp->type || isLeftOpTypedOperator(binOp)) {
        nodeType = leftOp->type;
    } else if (isIntTypedOperator(binOp)) {
        nodeType = VT_INT;
    }
    node->setInfo(new TypeInfo(nodeType));
}
