#include "ast_node_type_resolver.h"

#include <stack>

namespace mathvm {

namespace type_resolver {

namespace {

class TypeResolverVisitor: public AstVisitor {

public:
    TypeResolverVisitor(AstFunction* astTop) {
        astTop->node()->visit(this);
    }

    std::map<AstNode*, VarType> getResolver() const {
        return _resolver;
    }

#define VISITOR_FUNCTION(type, name) \
      virtual void visit##type(type* node);

      FOR_NODES(VISITOR_FUNCTION)

#undef VISITOR_FUNCTION

private:
    std::map<AstNode*, VarType> _resolver;
    std::stack<Scope*> _scopes;

};

void TypeResolverVisitor::visitBinaryOpNode(BinaryOpNode* node) {
    node->visitChildren(this);
    auto lt = _resolver[node->left()];
    auto rt = _resolver[node->right()];

    VarType type;

    switch (node->kind()) {
        case tASSIGN:
            type = lt;
            break;
        case tOR:
        case tAND:
        case tAOR:
        case tAAND:
        case tAXOR:
        case tRANGE:
        case tEQ:
        case tNEQ:
        case tGT:
        case tGE:
        case tLT:
        case tLE:
        case tMOD:
            type = VT_INT;
            break;
        case tADD:
        case tSUB:
        case tMUL:
        case tDIV:
            type = (lt == VT_DOUBLE || rt == VT_DOUBLE) ? VT_DOUBLE : VT_INT;
            break;
        default:
            type = VT_INVALID;
    }

    _resolver[node] = (lt == VT_STRING || rt == VT_STRING) ? VT_INVALID : type;
}

void TypeResolverVisitor::visitUnaryOpNode(UnaryOpNode *node) {
    node->visitChildren(this);
    auto child = node->operand();
    child->visit(this);
    if (node->kind() == tNOT) {
        _resolver[node] = VT_INT;
    } else {
        _resolver[node] = _resolver[child];
    }
}

void TypeResolverVisitor::visitStringLiteralNode(StringLiteralNode *node) {
    node->visitChildren(this);
    _resolver[node] = VT_STRING;
}

void TypeResolverVisitor::visitDoubleLiteralNode(DoubleLiteralNode *node) {
    node->visitChildren(this);
    _resolver[node] = VT_DOUBLE;
}

void TypeResolverVisitor::visitIntLiteralNode(IntLiteralNode *node) {
    node->visitChildren(this);
    _resolver[node] = VT_INT;
}

void TypeResolverVisitor::visitLoadNode(LoadNode *node) {
    node->visitChildren(this);
    _resolver[node] = node->var()->type();
}

void TypeResolverVisitor::visitStoreNode(StoreNode *node) {
    node->visitChildren(this);
    _resolver[node] = VT_INVALID;
}

void TypeResolverVisitor::visitForNode(ForNode *node) {
    node->visitChildren(this);
    _resolver[node] = VT_INVALID;
}

void TypeResolverVisitor::visitWhileNode(WhileNode *node) {
    node->visitChildren(this);
    _resolver[node] = VT_INVALID;
}

void TypeResolverVisitor::visitIfNode(IfNode *node) {
    node->visitChildren(this);
    _resolver[node] = VT_INVALID;
}

void TypeResolverVisitor::visitBlockNode(BlockNode *node) {
    _scopes.push(node->scope());
    node->visitChildren(this);
    auto funcIt = Scope::FunctionIterator(node->scope());
    while (funcIt.hasNext()) {
        funcIt.next()->node()->visit(this);
    }
    _resolver[node] = VT_INVALID;
    _scopes.pop();
}

void TypeResolverVisitor::visitFunctionNode(FunctionNode *node) {
    node->visitChildren(this);
    _resolver[node] = VT_INVALID;
}

void TypeResolverVisitor::visitReturnNode(ReturnNode *node) {
    node->visitChildren(this);
    _resolver[node] = VT_INVALID;
}

void TypeResolverVisitor::visitCallNode(CallNode *node) {
    node->visitChildren(this);
    if (_scopes.empty()) {
        _resolver[node] = VT_INVALID;
    } else {
        _resolver[node] = _scopes.top()->lookupFunction(node->name(), true)->returnType();
    }
}

void TypeResolverVisitor::visitNativeCallNode(NativeCallNode *node) {
    node->visitChildren(this);
    const auto& signature = node->nativeSignature();
    if (signature.empty()) {
        _resolver[node] = VT_INVALID;
    } else {
        _resolver[node] = signature[0].first;
    }
}

void TypeResolverVisitor::visitPrintNode(PrintNode *node) {
    node->visitChildren(this);
    _resolver[node] = VT_VOID;
}

} // anonymous namespace

AstNodeTypeResolver::AstNodeTypeResolver(AstFunction* astTop) {
    _resolver = TypeResolverVisitor(astTop).getResolver();
}

VarType AstNodeTypeResolver::operator() (AstNode* node) const {
        return _resolver.at(node);
    }

} // namspace type_resolver

} // namespace mathvm

