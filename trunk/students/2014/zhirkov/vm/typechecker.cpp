#include <string.h>
#include "typechecker.h"

#include "../../../../include/ast.h"

namespace mathvm {


    void TypeChecker::visitAstFunction(AstFunction* fun) {

    }

    static bool isValid(VarType t) { return t != VT_INVALID; }
    static bool isNumeric(VarType t) { return t == VT_INT || t == VT_DOUBLE; }

    static VarType binopType(TokenKind type, VarType l, VarType r) {
        if ( !isValid(l) || !isValid(r) || !isNumeric(l) || !isNumeric(r))
            return VT_INVALID;

        switch(type) {
            case tADD:
            case tSUB:
            case tMUL:
            case tDIV:
                if ((l == VT_DOUBLE) || r == VT_DOUBLE) return VT_DOUBLE;
                if (l == VT_INT &&  r == VT_INT) return VT_INT;
            case tMOD:
                return VT_INT;
            case tAOR: case tAAND: case tAND: case tOR: case tAXOR: case tEQ: case tNEQ: case tGT: case tGE:
            case tLT: case tLE:
                return VT_INT;
        }
        return VT_INVALID;
    }

    void TypeChecker::visitBinaryOpNode(BinaryOpNode *node) {
        node->visitChildren(this);
        auto l = getType(node->left());
        auto r = getType(node->right());

        setType(node, binopType(node->kind(), l, r));
    }

    void TypeChecker::visitUnaryOpNode(UnaryOpNode *node) {
        node->visitChildren(this);
        if (! isValid(getData(node->operand()).type))
            getData(node).type = VT_INVALID;
        switch(node->kind()) {
            case tNOT:
                setType(node, VT_INT); break;
            case tSUB:
                setType(node, getType(node->operand())); break;
            default:
            std::cerr<< "Typechecker: Can not assign type to unop node: unknown node type" << tokenOp(node->kind());
                break;
        }
    }

    void TypeChecker::visitStringLiteralNode(StringLiteralNode *node) {
        setType(node, VT_STRING);
    }
    void TypeChecker::visitDoubleLiteralNode(DoubleLiteralNode *node) {setType(node, VT_DOUBLE); }
    void TypeChecker::visitIntLiteralNode(IntLiteralNode *node) { setType(node, VT_INT); }
    void TypeChecker::visitLoadNode(LoadNode *node) {
        setType(node, node->var()->type());
    }

    void TypeChecker::visitStoreNode(StoreNode *node) {
        node->visitChildren(this);
        setType(node, VT_VOID);
    }
    void TypeChecker::visitForNode(ForNode *node) {
        node->visitChildren(this);
        setType(node, VT_VOID);
    }
    void TypeChecker::visitWhileNode(WhileNode *node) {
        node->visitChildren(this);
        setType(node, VT_VOID);
    }

    void TypeChecker::visitIfNode(IfNode *node) {
        node->visitChildren(this);
        setType(node, VT_VOID);
    }
    void TypeChecker::visitBlockNode(BlockNode *node) {
        scope = node->scope();
        node->visitChildren(this);
        setType(node, VT_VOID);
        scope = scope->parent();
    }

    void TypeChecker::visitFunctionNode(FunctionNode *node) {
        node->visitChildren(this);
        setType(node, VT_VOID);
    }
    void TypeChecker::visitReturnNode(ReturnNode *node) {
        node->visitChildren(this);
        setType(node, VT_VOID);
    }
    void TypeChecker::visitCallNode(CallNode *node) {
        node->visitChildren(this);
        auto fun = scope->lookupFunction(node->name(), true);
        if (fun)
        setType(node, fun->returnType());
        else {
            std::cerr << "Typechecker: Can't find function " << node->name();
            setType(node, VT_INVALID);
        }
    }

    void TypeChecker::visitNativeCallNode(NativeCallNode *node) {
        std::cerr <<"Typechecker: native calls not implemented" << std::endl;
    }
    void TypeChecker::visitPrintNode(PrintNode *node) {
        node->visitChildren(this);
        setType(node, VT_INVALID);
    }

}