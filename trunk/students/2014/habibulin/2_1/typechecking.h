#ifndef TYPECHECKING_H
#define TYPECHECKING_H

#include <string>
#include <stack>

#include "mathvm.h"
#include "ast.h"

#include "my_utils.h"

using std::string;
using std::stack;
using namespace mathvm;
using std::to_string;

struct TypeInfo {
    VarType type;

    TypeInfo(VarType vt): type(vt) {}
};

inline bool nodeIsTyped(AstNode* node) {
    return node->info() != 0;
}

inline TypeInfo* nodeTypeInfo(AstNode* node) {
    if(node->info() == 0) {
        throw ExceptionWithPos("expr with no type is encountered", node->position());
    }
    return (TypeInfo *)node->info();
}

inline VarType nodeType(AstNode* node) {
    return nodeTypeInfo(node)->type;
}

inline bool isIntDoublePair(VarType fst, VarType snd) {
    return (fst == VT_INT && snd == VT_DOUBLE) || (fst == VT_DOUBLE && snd == VT_INT);
}

class TypeChecker: public AstVisitor {
private:
    ExecStatus _status;
    stack<AstFunction*> _funs;

public:
    bool check(AstFunction* astFun);
    ExecStatus const & status() { return _status; }

    virtual void visitFunctionNode(FunctionNode* node);
    virtual void visitBlockNode(BlockNode* node);
    virtual void visitReturnNode(ReturnNode* node);
    virtual void visitNativeCallNode(NativeCallNode* node);
    virtual void visitCallNode(CallNode* node);
    virtual void visitPrintNode(PrintNode* node);
    virtual void visitBinaryOpNode(BinaryOpNode* node);
    virtual void visitUnaryOpNode(UnaryOpNode* node);
    virtual void visitStringLiteralNode(StringLiteralNode* node);
    virtual void visitIntLiteralNode(IntLiteralNode* node);
    virtual void visitDoubleLiteralNode(DoubleLiteralNode* node);
    virtual void visitLoadNode(LoadNode* node);
    virtual void visitStoreNode(StoreNode* node);
    virtual void visitForNode(ForNode* node);
    virtual void visitWhileNode(WhileNode* node);
    virtual void visitIfNode(IfNode* node);

private:
    // check impl
    void visitBlockBody(BlockNode* node);
    void visitFunDefs(BlockNode* node);
    void visitExprs(BlockNode* node);

    // visitReturnNode impl
    string invalidRetExprTypeMsg(VarType expected, VarType actual) {
        string const exp = string(typeToName(expected));
        string const act = string(typeToName(actual));
        return "return expr has wrong type, expected '" + exp + "', actual '" + act + "'";
    }
    string retExprExpectedMsg() {
        return "empty 'return' expr in function returning non-void";
    }

    // visitCallNode impl
    Scope* currentScope() {
        assert(_funs.size() != 0);
        return _funs.top()->node()->body()->scope();
    }
    string wrongArgTypeMsg(string const& fun, size_t opNum) {
        return "function '" + fun + "' call - argument " + to_string(opNum) +
               " has wrong type";
    }
    string undeclaredFunMsg(string const& name) {
        return "undeclared function '" + name + "'";
    }

    // visitBinaryOpNode impl
    string nodeIsNotTyped() {
        return "internal error - not typed node encountered";
    }
    void checkTypesAreCompatible(TypeInfo* leftOp, TypeInfo* rightOp, size_t errPos);
    string binOpWrongTypeMsg(VarType wrongType) {
        string const tName = string(typeToName(wrongType));
        return "operand of bin op has wrong type '" + tName + "'";
    }
    string binOpStringAndNonStringMsg() {
        return "bin op is applied to 'string' and 'non-string' operands";
    }
    void checkOperandsAreCorrect(TypeInfo* leftOp, TypeInfo* rightOp, TokenKind binOp, size_t errPos);
    bool isIntsExpectingOp(TokenKind binOp) {
        switch (binOp) {
        case tAOR:
        case tAAND:
        case tAXOR:
        case tMOD:
        case tRANGE: return true;
        default:     return false;
        }
    }

    string invalidBinOpOnStrMsg(TokenKind binOp) {
        string const binOpName = string(tokenOp(binOp));
        return "invalid bin op '" + binOpName + "' on 'string' operands";
    }
    string invalidBinOpOnDoubleMsg(TokenKind binOp) {
        string const binOpName = string(tokenOp(binOp));
        return "invalid bin op '" + binOpName + "' on 'double' operand(s)";
    }
    void setBinOpNodeType(AstNode* node, TypeInfo* leftOp, TypeInfo* rightOp, TokenKind binOp);
    bool isIntTypedOperator(TokenKind binOp) {
        switch (binOp) {
        case tOR:
        case tAND:
        case tAOR:
        case tAAND:
        case tAXOR:
        case tMOD: return true;
        default:   return false;
        }
    }
    bool isLeftOpTypedOperator(TokenKind binOp) {
        switch (binOp) {
        case tASSIGN:
        case tINCRSET:
        case tDECRSET: return true;
        default:       return false;
        }
    }

    // visitUnaryOpNode impl
    string unaryOpWrongTypeMsg(VarType wrongType) {
        string const tName = string(typeToName(wrongType));
        return "operand of unary op has wrong type '" + tName + "'";
    }
    string notOpOnDoubleMsg() {
        return "not operator '!' is applied to double operand";
    }

    // visitLoadNode impl
    string loadNodeInvalidTypeMsg(VarType invalidType) {
        string const typeName = string(typeToName(invalidType));
        return "variable has invalid type '" + typeName + "'";
    }

    // visitStoreNode impl
    string storeNodeInvalidOpTypesMsg() {
        return "assignment is applied to wrong type operands (both must be numeric or string)";
    }
    string compoundAssignOpOnStrMsg() {
        return "compound assignment operator is applied to 'string' operands";
    }

    // visitForNode impl
    string iterVarWrongTypeMsg() {
        return "'in' operator - iter var must have type 'int'";
    }

    // visitWhileNode impl
    string condExprWrongTypeMsg() {
        return "conditional expr must have type 'int'";
    }
};

class InfoDeletter: public AstVisitor {
public:
    void run(AstFunction* root) {
        if(root) {
            root->node()->visit(this);
        }
    }

    virtual void visitFunctionNode(FunctionNode* node) { generalDel(node); }
    virtual void visitBlockNode(BlockNode* node) {
        Scope::FunctionIterator funIt(node->scope());
        while(funIt.hasNext()) {
            AstFunction* fun = funIt.next();
            fun->node()->visit(this);
        }
        generalDel(node);
    }
    virtual void visitReturnNode(ReturnNode* node) { generalDel(node); }
    virtual void visitNativeCallNode(NativeCallNode* node) { generalDel(node); }
    virtual void visitCallNode(CallNode* node) { generalDel(node); }
    virtual void visitPrintNode(PrintNode* node) { generalDel(node); }
    virtual void visitBinaryOpNode(BinaryOpNode* node) { generalDel(node); }
    virtual void visitUnaryOpNode(UnaryOpNode* node) { generalDel(node); }
    virtual void visitStringLiteralNode(StringLiteralNode* node) { generalDel(node); }
    virtual void visitIntLiteralNode(IntLiteralNode* node) { generalDel(node); }
    virtual void visitDoubleLiteralNode(DoubleLiteralNode* node) { generalDel(node); }
    virtual void visitLoadNode(LoadNode* node) { generalDel(node); }
    virtual void visitStoreNode(StoreNode* node) { generalDel(node); }
    virtual void visitForNode(ForNode* node) { generalDel(node); }
    virtual void visitWhileNode(WhileNode* node) { generalDel(node); }
    virtual void visitIfNode(IfNode* node) { generalDel(node); }
private:
    void generalDel(AstNode* node) {
        node->visitChildren(this);
        if(node->info() != 0) {
            delete ((TypeInfo *)node->info());
        }
    }
};


#endif // TYPECHECKING_H
