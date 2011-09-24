#ifndef _MATHVM_AST_H
#define _MATHVM_AST_H

#include <map>

#include "mathvm.h"

namespace mathvm {

#define FOR_TOKENS(DO)                          \
    DO(tEOF, "", 0)                             \
    DO(tLPAREN, "(", 0)                         \
    DO(tRPAREN, ")", 0)                         \
    DO(tLBRACE, "{", 0)                         \
    DO(tRBRACE, "}", 0)                         \
    DO(tASSIGN, "=", 2)                         \
    DO(tOR, "||", 4)                            \
    DO(tAND, "&&", 5)                           \
    DO(tNOT, "!", 0)                            \
    DO(tEQ,     "==", 9)                        \
    DO(tNEQ,    "!=", 9)                        \
    DO(tGT,     ">", 10)                        \
    DO(tGE,     ">=", 10)                       \
    DO(tLT,     "<", 10)                        \
    DO(tLE,     "<=", 10)                       \
    DO(tRANGE, "..", 12)                        \
    DO(tADD, "+", 12)                           \
    DO(tSUB, "-", 12)                           \
    DO(tMUL, "*", 13)                           \
    DO(tDIV, "/", 13)                           \
    DO(tINCRSET, "+=", 14)                      \
    DO(tDECRSET, "-=", 14)                      \
    DO(tDOUBLE, "", 0)                          \
    DO(tINT, "", 0)                             \
    DO(tSTRING, "", 0)                          \
    DO(tCOMMA, ",", 0)                          \
    DO(tSEMICOLON, ";", 0)                      \
    DO(tIDENT, "", 0)                           \
    DO(tERROR, "", 0)                           \
    DO(tUNDEF, "", 0)

enum TokenKind {
#define ENUM_ELEM(t, s, p) t,
    FOR_TOKENS(ENUM_ELEM)
    tTokenCount
#undef ENUM_ELEM
};

#define FOR_KEYWORDS(DO)                        \
    DO("int")                                   \
    DO("double")                                \
    DO("string")                                \
    DO("for")                                   \
    DO("while")                                 \
    DO("if")                                    \
    DO("print")                                 \
    DO("function")

static inline bool isKeyword(const string& word) {
#define CHECK_WORD(w) if (word == w) return true;
    FOR_KEYWORDS(CHECK_WORD);
    return false;
}

#define FOR_NODES(DO)                           \
            DO(BinaryOpNode, "binary")          \
            DO(UnaryOpNode, "unary")            \
            DO(StringLiteralNode, "string literal") \
            DO(DoubleLiteralNode, "double literal") \
            DO(IntLiteralNode, "int literal")   \
            DO(LoadNode, "loadval")             \
            DO(StoreNode, "storeval")           \
            DO(ForNode, "for")                  \
            DO(WhileNode, "while")              \
            DO(IfNode, "if")                    \
            DO(BlockNode, "block")              \
            DO(FunctionNode, "function")        \
            DO(PrintNode, "print")

#define FORWARD_DECLARATION(type, name) class type;
FOR_NODES(FORWARD_DECLARATION)
#undef FORWARD_DECLARATION

class Scope;
class AstVar {
    const string _name;
    VarType _type;
    Scope* _owner;
  public:
  AstVar(const string& name, VarType type, Scope* owner) :
    _name(name), _type(type), _owner(owner) {
    }

    const string& name() const {
        return _name;
    }

    VarType type() const {
        return _type;
    }

    Scope* owner() const {
        return _owner;
    }
};

class Scope {
    typedef std::map<string, AstVar* > VarMap;
    VarMap _vars;
    Scope* _parent;

  public:
    Scope(Scope* parent): _parent(parent) {}
    ~Scope();

    void declare(const string& name, VarType type);
    AstVar* lookup(const string& name);
    Scope* parent() const { return _parent; }

    class VarIterator {
        VarMap::iterator _it;
        Scope* _scope;
        bool _includeOuter;
    public:
        VarIterator(Scope* scope, bool includeOuter = false) :
            _scope(scope), _includeOuter(includeOuter) {
            _it = _scope->_vars.begin();
        }

        bool hasNext();
        AstVar* next();
    };
};

class AstVisitor {
  public:
    AstVisitor() {
    }
    virtual ~AstVisitor() {
    }

#define VISITOR_FUNCTION(type, name) \
    virtual void visit##type(type* node) {}

    FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION
};

#define COMMON_NODE_FUNCTIONS(type)                           \
    virtual void visit(AstVisitor* visitor);                  \
    virtual bool is##type() const { return true; }            \
    virtual type* as##type() { return this; }

class AstNode {
    uint32_t _index;
  public:
    AstNode(uint32_t index) :
        _index(index) {
    }
    virtual ~AstNode() {
    }

    virtual void visit(AstVisitor* visitor) = 0;
    virtual void visitChildren(AstVisitor* visitor) const = 0;

#define CHECK_FUNCTION(type, name)                  \
    virtual bool is##type() const { return false; } \
    virtual type* as##type() { return 0; }

    FOR_NODES(CHECK_FUNCTION)
#undef CHECK_FUNCTION

};

class BinaryOpNode : public AstNode {
  const TokenKind _kind;
  AstNode* _left;
  AstNode* _right;
 public:
  BinaryOpNode(uint32_t index,
               TokenKind kind,
               AstNode* left,
               AstNode* right) :
      AstNode(index), _kind(kind), _left(left), _right(right) {
    assert(_left != 0);
    assert(_right != 0);
  }

  TokenKind kind() const { return _kind; }
  AstNode* left() const { return _left; }
  AstNode* right() const { return _right; }

  virtual void visitChildren(AstVisitor* visitor) const {
    left()->visit(visitor);
    right()->visit(visitor);
  }

  COMMON_NODE_FUNCTIONS(BinaryOpNode);
};

class UnaryOpNode : public AstNode {
    const TokenKind _kind;
    AstNode* _operand;
  public:
    UnaryOpNode(uint32_t index,
                TokenKind kind,
                AstNode* operand) :
      AstNode(index), _kind(kind), _operand(operand) {
        assert(_operand != 0);
    }

    TokenKind kind() const { return _kind; }
    AstNode* operand() const { return _operand; }

    virtual void visitChildren(AstVisitor* visitor) const {
        operand()->visit(visitor);
    }

    COMMON_NODE_FUNCTIONS(UnaryOpNode);
};


class StringLiteralNode : public AstNode {
    const string _stringLiteral;
  public:
    StringLiteralNode(uint32_t index, const string& stringLiteral) :
        AstNode(index), _stringLiteral(stringLiteral) {
    }

    const string& literal() const {
        return _stringLiteral;
    }

    virtual void visitChildren(AstVisitor* visitor) const {
    }

    COMMON_NODE_FUNCTIONS(StringLiteralNode);
};

class IntLiteralNode : public AstNode {
    const int64_t _intLiteral;
  public:
    IntLiteralNode(uint32_t index, int64_t intLiteral) :
    AstNode(index), _intLiteral(intLiteral) {
    }

    const int64_t literal() const {
        return _intLiteral;
    }

    virtual void visitChildren(AstVisitor* visitor) const {
    }

    COMMON_NODE_FUNCTIONS(IntLiteralNode);
};

class DoubleLiteralNode : public AstNode {
    const double _doubleLiteral;
  public:
    DoubleLiteralNode(uint32_t index, double doubleLiteral) :
    AstNode(index), _doubleLiteral(doubleLiteral) {
    }

    double literal() const {
        return _doubleLiteral;
    }

    virtual void visitChildren(AstVisitor* visitor) const {
    }

    COMMON_NODE_FUNCTIONS(DoubleLiteralNode);
};

class LoadNode : public AstNode {
    const AstVar* _var;
  public:
    LoadNode(uint32_t index, const AstVar* var) :
        AstNode(index), _var(var) {
    }

    const AstVar* var() const {
        return _var;
    }

    virtual void visitChildren(AstVisitor* visitor) const {
    }

    COMMON_NODE_FUNCTIONS(LoadNode);
};

class StoreNode : public AstNode {
    const AstVar* _var;
    AstNode* _value;
    TokenKind _op;

  public:
    StoreNode(uint32_t index,
              const AstVar* var,
              AstNode* value,
              TokenKind op) :
    AstNode(index), _var(var), _value(value), _op(op) {
        assert(_value != 0);
        assert(_op == tASSIGN || _op == tINCRSET || _op == tDECRSET);
    }

    const AstVar* var() const {
        return _var;
    }

    AstNode* value() const {
        return _value;
    }

    TokenKind op() const {
        return _op;
    }

    virtual void visitChildren(AstVisitor* visitor) const {
        value()->visit(visitor);
    }

    COMMON_NODE_FUNCTIONS(StoreNode);
};

class Scope;

class BlockNode : public AstNode {
    vector<AstNode*> _nodes;
    Scope* _scope;

  public:
    BlockNode(uint32_t index,
              Scope* scope) :
    AstNode(index), _scope(scope) {
    }

    ~BlockNode();

    Scope* scope() const {
        return _scope;
    }

    uint32_t nodes() const {
        return _nodes.size();
    }

    AstNode* nodeAt(uint32_t index) const {
        return _nodes[index];
    }

    void add(AstNode* node) {
        _nodes.push_back(node);
    }

    virtual void visitChildren(AstVisitor* visitor) const {
        for (uint32_t i = 0; i < nodes(); i++) {
            nodeAt(i)->visit(visitor);
        }
    }

    COMMON_NODE_FUNCTIONS(BlockNode);
};

class ForNode : public AstNode {
    const AstVar* _var;
    AstNode* _inExpr;
    BlockNode* _body;

  public:
    ForNode(uint32_t index,
            const AstVar* var,
            AstNode* inExpr,
            BlockNode* body) :
    AstNode(index), _var(var), _inExpr(inExpr), _body(body) {
        assert(_inExpr != 0);
        assert(_body != 0);
    }

    const AstVar* var() const {
        return _var;
    }

    AstNode* inExpr() const {
        return _inExpr;
    }

    BlockNode* body() const {
        return _body;
    }

    virtual void visitChildren(AstVisitor* visitor) const {
        inExpr()->visit(visitor);
        body()->visit(visitor);
    }

    COMMON_NODE_FUNCTIONS(ForNode);
};

class WhileNode : public AstNode {
    AstNode* _whileExpr;
    BlockNode* _loopBlock;

  public:
    WhileNode(uint32_t index,
              AstNode* whileExpr,
              BlockNode* loopBlock) :
    AstNode(index), _whileExpr(whileExpr), 
    _loopBlock(loopBlock) {
        assert(_whileExpr != 0);
        assert(_loopBlock != 0);
    }

    AstNode* whileExpr() const {
        return _whileExpr;
    }

    BlockNode* loopBlock() const {
        return _loopBlock;
    }

    virtual void visitChildren(AstVisitor* visitor) const {
        whileExpr()->visit(visitor);
        loopBlock()->visit(visitor);
    }

    COMMON_NODE_FUNCTIONS(WhileNode);
};

class IfNode : public AstNode {
    AstNode* _ifExpr;
    BlockNode* _thenBlock;
    BlockNode* _elseBlock;

  public:
    IfNode(uint32_t index,
           AstNode* ifExpr,
           BlockNode* thenBlock,
           BlockNode* elseBlock) :
    AstNode(index), _ifExpr(ifExpr), 
    _thenBlock(thenBlock), _elseBlock(elseBlock) {
        assert(_ifExpr != 0);
        assert(_thenBlock != 0);
    }

    AstNode* ifExpr() const {
        return _ifExpr;
    }

    BlockNode* thenBlock() const {
        return _thenBlock;
    }

    BlockNode* elseBlock() const {
        return _elseBlock;
    }

    virtual void visitChildren(AstVisitor* visitor) const {
        ifExpr()->visit(visitor);
        thenBlock()->visit(visitor);
        if (elseBlock()) {
            elseBlock()->visit(visitor);
        }
    }

    COMMON_NODE_FUNCTIONS(IfNode);
};

class FunctionNode : public AstNode {
    const string _name;
    AstNode* _args;
    BlockNode* _body;

  public:
    FunctionNode(uint32_t index,
                 const string& name,
                 AstNode* args,
                 BlockNode* body) :
    AstNode(index), _name(name), _args(args), _body(body) {
        assert(_args != 0);
        assert(_body != 0);
    }

    const string& name() const {
        return _name;
    }

    AstNode* args() const {
        return _args;
    }

    BlockNode* body() const {
        return _body;
    }

    virtual void visitChildren(AstVisitor* visitor) const {
        args()->visit(visitor);
        body()->visit(visitor);
    }

    COMMON_NODE_FUNCTIONS(FunctionNode);
};

class PrintNode : public AstNode {
    vector<AstNode*> _operands;

  public:
    PrintNode(uint32_t index) :
    AstNode(index) {
    }

    uint32_t operands() const {
        return _operands.size();
    }

    AstNode* operandAt(uint32_t index) const {
        return _operands[index];
    }

    void add(AstNode* node) {
        _operands.push_back(node);
    }

    virtual void visitChildren(AstVisitor* visitor) const {
        for (uint32_t i = 0; i < operands(); i++) {
            operandAt(i)->visit(visitor);
        }
    }

    COMMON_NODE_FUNCTIONS(PrintNode);
};

#undef COMMON_NODE_FUNCTIONS

static inline const char* tokenStr(TokenKind token) {
#define R(t, s, p) case t: return #t;
        switch (token) {
            FOR_TOKENS(R)
        default:
            return "<unknown>";
#undef R
        }
}

static inline const char* tokenOp(TokenKind token) {
#define R(t, s, p) case t: return s;
        switch (token) {
            FOR_TOKENS(R)
        default:
            return "???";
#undef R
        }
}

static inline int tokenPrecedence(TokenKind token) {
#define R(t, s, p) case t: return p;
        switch (token) {
            FOR_TOKENS(R)
        default:
            return 0;
#undef R
        }
}

}
#endif
