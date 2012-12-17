# 1 "ast.c"
# 1 "<built-in>"
# 1 "<command-line>"
# 1 "ast.c"



namespace mathvm {
# 39 "ast.c"
enum TokenKind {

    tEOF, tLPAREN, tRPAREN, tLBRACE, tRBRACE, tASSIGN, tOR, tAND, tNOT, tEQ, tNEQ, tGT, tGE, tLT, tLE, tRANGE, tADD, tSUB, tMUL, tDIV, tMOD, tINCRSET, tDECRSET, tDOUBLE, tINT, tSTRING, tCOMMA, tSEMICOLON, tIDENT, tERROR, tUNDEF,
    tTokenCount

};
# 58 "ast.c"
static inline bool isKeyword(const string& word) {

    if (word == "int") return true; if (word == "double") return true; if (word == "string") return true; if (word == "for") return true; if (word == "while") return true; if (word == "if") return true; if (word == "print") return true; if (word == "function") return true; if (word == "native") return true; if (word == "return") return true;;
    return false;
}
# 83 "ast.c"
class BinaryOpNode; class UnaryOpNode; class StringLiteralNode; class DoubleLiteralNode; class IntLiteralNode; class LoadNode; class StoreNode; class ForNode; class WhileNode; class IfNode; class BlockNode; class FunctionNode; class ReturnNode; class CallNode; class NativeCallNode; class PrintNode;


class Scope;

class CustomDataHolder {
    void* _info;
  public:
    void* info() const { return _info; }
    void setInfo(void* info) { _info = info; }
  protected:
    CustomDataHolder() : _info(0) {}
};
# 104 "ast.c"
class AstVar : public CustomDataHolder {
    const string _name;
    VarType _type;
    Scope* _owner;
  public:
    AstVar(const string& name, VarType type, Scope* owner) :
    _name(name), _type(type), _owner(owner) {
    }
    const string& name() const { return _name; }
    VarType type() const { return _type; }
    Scope* owner() const { return _owner; }
};

class FunctionNode;
class AstFunction : public CustomDataHolder {
    FunctionNode* _function;
    Scope* _owner;
  public:
    AstFunction(FunctionNode* function, Scope* owner) :
        _function(function), _owner(owner) {
          assert(_function != 0);
    }
    ~AstFunction();

    const string& name() const;
    VarType returnType() const;
    VarType parameterType(uint32_t index) const;
    const string& parameterName(uint32_t index) const;
    uint32_t parametersNumber() const;
    Scope* owner() const { return _owner; }
    Scope* scope() const;
    FunctionNode* node() const { return _function; }
    static const string top_name;
    static const string invalid;
};

class Scope {
    typedef std::map<string, AstVar* > VarMap;
    typedef std::map<string, AstFunction* > FunctionMap;

    VarMap _vars;
    FunctionMap _functions;
    Scope* _parent;
    vector<Scope*> _children;

  public:
    Scope(Scope* parent): _parent(parent) {}
    ~Scope();

    uint32_t childScopeNumber() { return _children.size(); }
    Scope* childScopeAt(uint32_t index) { return _children[index]; }
    void addChildScope(Scope* scope) { _children.push_back(scope); }

    bool declareVariable(const string& name, VarType type);
    bool declareFunction(FunctionNode* node);

    AstVar* lookupVariable(const string& name, bool useParent = true);
    AstFunction* lookupFunction(const string& name, bool useParent = true);

    uint32_t variablesCount() const { return _vars.size(); }
    uint32_t functionsCount() const { return _functions.size(); }

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

    class FunctionIterator {
        FunctionMap::iterator _it;
        Scope* _scope;
        bool _includeOuter;
    public:
        FunctionIterator(Scope* scope, bool includeOuter = false) :
            _scope(scope), _includeOuter(includeOuter) {
            _it = _scope->_functions.begin();
        }

        bool hasNext();
        AstFunction* next();
    };

};

class AstVisitor {
  public:
    AstVisitor() {
    }
    virtual ~AstVisitor() {
    }




    virtual void visitBinaryOpNode(BinaryOpNode* node) {} virtual void visitUnaryOpNode(UnaryOpNode* node) {} virtual void visitStringLiteralNode(StringLiteralNode* node) {} virtual void visitDoubleLiteralNode(DoubleLiteralNode* node) {} virtual void visitIntLiteralNode(IntLiteralNode* node) {} virtual void visitLoadNode(LoadNode* node) {} virtual void visitStoreNode(StoreNode* node) {} virtual void visitForNode(ForNode* node) {} virtual void visitWhileNode(WhileNode* node) {} virtual void visitIfNode(IfNode* node) {} virtual void visitBlockNode(BlockNode* node) {} virtual void visitFunctionNode(FunctionNode* node) {} virtual void visitReturnNode(ReturnNode* node) {} virtual void visitCallNode(CallNode* node) {} virtual void visitNativeCallNode(NativeCallNode* node) {} virtual void visitPrintNode(PrintNode* node) {}

};






class AstNode : public CustomDataHolder {
    uint32_t _index;
  public:
    AstNode(uint32_t index) :
        _index(index) {
    }
    virtual ~AstNode() {
    }

    virtual void visit(AstVisitor* visitor) = 0;
    virtual void visitChildren(AstVisitor* visitor) const = 0;
    uint32_t position() const {
        return _index;
    }





    virtual bool isBinaryOpNode() const { return false; } virtual BinaryOpNode* asBinaryOpNode() { return 0; } virtual bool isUnaryOpNode() const { return false; } virtual UnaryOpNode* asUnaryOpNode() { return 0; } virtual bool isStringLiteralNode() const { return false; } virtual StringLiteralNode* asStringLiteralNode() { return 0; } virtual bool isDoubleLiteralNode() const { return false; } virtual DoubleLiteralNode* asDoubleLiteralNode() { return 0; } virtual bool isIntLiteralNode() const { return false; } virtual IntLiteralNode* asIntLiteralNode() { return 0; } virtual bool isLoadNode() const { return false; } virtual LoadNode* asLoadNode() { return 0; } virtual bool isStoreNode() const { return false; } virtual StoreNode* asStoreNode() { return 0; } virtual bool isForNode() const { return false; } virtual ForNode* asForNode() { return 0; } virtual bool isWhileNode() const { return false; } virtual WhileNode* asWhileNode() { return 0; } virtual bool isIfNode() const { return false; } virtual IfNode* asIfNode() { return 0; } virtual bool isBlockNode() const { return false; } virtual BlockNode* asBlockNode() { return 0; } virtual bool isFunctionNode() const { return false; } virtual FunctionNode* asFunctionNode() { return 0; } virtual bool isReturnNode() const { return false; } virtual ReturnNode* asReturnNode() { return 0; } virtual bool isCallNode() const { return false; } virtual CallNode* asCallNode() { return 0; } virtual bool isNativeCallNode() const { return false; } virtual NativeCallNode* asNativeCallNode() { return 0; } virtual bool isPrintNode() const { return false; } virtual PrintNode* asPrintNode() { return 0; }


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

  virtual void visit(AstVisitor* visitor); virtual bool isBinaryOpNode() const { return true; } virtual BinaryOpNode* asBinaryOpNode() { return this; };
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

    virtual void visit(AstVisitor* visitor); virtual bool isUnaryOpNode() const { return true; } virtual UnaryOpNode* asUnaryOpNode() { return this; };
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

    virtual void visit(AstVisitor* visitor); virtual bool isStringLiteralNode() const { return true; } virtual StringLiteralNode* asStringLiteralNode() { return this; };
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

    virtual void visit(AstVisitor* visitor); virtual bool isIntLiteralNode() const { return true; } virtual IntLiteralNode* asIntLiteralNode() { return this; };
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

    virtual void visit(AstVisitor* visitor); virtual bool isDoubleLiteralNode() const { return true; } virtual DoubleLiteralNode* asDoubleLiteralNode() { return this; };
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

    virtual void visit(AstVisitor* visitor); virtual bool isLoadNode() const { return true; } virtual LoadNode* asLoadNode() { return this; };
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

    virtual void visit(AstVisitor* visitor); virtual bool isStoreNode() const { return true; } virtual StoreNode* asStoreNode() { return this; };
};

class BlockNode : public AstNode {
    vector<AstNode*> _nodes;
    Scope* _scope;

  public:
    BlockNode(uint32_t index,
              Scope* scope) :
    AstNode(index), _scope(scope) {
    }

    ~BlockNode() {
    }

    Scope* scope() const {
        return _scope;
    }

    virtual uint32_t nodes() const {
        return _nodes.size();
    }

    virtual AstNode* nodeAt(uint32_t index) const {
        return _nodes[index];
    }

    virtual void add(AstNode* node) {
        _nodes.push_back(node);
    }

    virtual void visitChildren(AstVisitor* visitor) const {
        for (uint32_t i = 0; i < nodes(); i++) {
            nodeAt(i)->visit(visitor);
        }
    }

    virtual void visit(AstVisitor* visitor); virtual bool isBlockNode() const { return true; } virtual BlockNode* asBlockNode() { return this; };
};

class NativeCallNode : public AstNode {
    string _nativeName;
    Signature _signature;
  public:
    NativeCallNode(uint32_t index,
                   const string& nativeName,
                   vector<pair<VarType,string> >& signature) :
    AstNode(index), _nativeName(nativeName), _signature(signature) {
    }

    ~NativeCallNode() {
    }

    const string& nativeName() const {
        return _nativeName;
    }

    const Signature& nativeSignature() const {
        return _signature;
    }


    virtual void visitChildren(AstVisitor* visitor) const {
    }

    virtual void visit(AstVisitor* visitor); virtual bool isNativeCallNode() const { return true; } virtual NativeCallNode* asNativeCallNode() { return this; };
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

    virtual void visit(AstVisitor* visitor); virtual bool isForNode() const { return true; } virtual ForNode* asForNode() { return this; };
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

    virtual void visit(AstVisitor* visitor); virtual bool isWhileNode() const { return true; } virtual WhileNode* asWhileNode() { return this; };
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

    virtual void visit(AstVisitor* visitor); virtual bool isIfNode() const { return true; } virtual IfNode* asIfNode() { return this; };
};

class ReturnNode : public AstNode {
    AstNode* _returnExpr;

  public:
    ReturnNode(uint32_t index,
              AstNode* returnExpr) :
    AstNode(index), _returnExpr(returnExpr) {
    }

    AstNode* returnExpr() const {
        return _returnExpr;
    }

    virtual void visitChildren(AstVisitor* visitor) const {
        if (returnExpr()) {
            returnExpr()->visit(visitor);
        }
    }

    virtual void visit(AstVisitor* visitor); virtual bool isReturnNode() const { return true; } virtual ReturnNode* asReturnNode() { return this; };
};

class FunctionNode : public AstNode {
    const string _name;
    Signature _signature;
    BlockNode* _body;

  public:
    FunctionNode(uint32_t index,
                 const string& name,
                 Signature& signature,
                 BlockNode* body) :
    AstNode(index), _name(name), _signature(signature), _body(body) {
        assert(_body != 0);
        assert(signature.size() > 0);
    }

    const string& name() const {
        return _name;
    }

    VarType returnType() const {
        return _signature[0].first;
    }

    uint32_t parametersNumber() const {
        return _signature.size() - 1;
    }

    VarType parameterType(uint32_t index) const {
        return _signature[index + 1].first;
    }

    const string& parameterName(uint32_t index) const {
        return _signature[index + 1].second;
    }

    const Signature& signature() const {
        return _signature;
    }

    BlockNode* body() const {
        return _body;
    }

    virtual void visitChildren(AstVisitor* visitor) const {
        body()->visit(visitor);
    }

    virtual void visit(AstVisitor* visitor); virtual bool isFunctionNode() const { return true; } virtual FunctionNode* asFunctionNode() { return this; };
};

class CallNode : public AstNode {
    const string _name;
    vector<AstNode*> _parameters;

public:
   CallNode(uint32_t index,
            const string& name,
            vector<AstNode*>& parameters) :
       AstNode(index), _name(name) {
        for (uint32_t i = 0; i < parameters.size(); i++) {
          _parameters.push_back(parameters[i]);
        }
    }

    const string& name() const {
        return _name;
    }

    uint32_t parametersNumber() const {
        return _parameters.size();
    }

    AstNode* parameterAt(uint32_t index) const {
        return _parameters[index];
    }

    virtual void visitChildren(AstVisitor* visitor) const {
        for (uint32_t i = 0; i < parametersNumber(); i++) {
            parameterAt(i)->visit(visitor);
        }
    }

    virtual void visit(AstVisitor* visitor); virtual bool isCallNode() const { return true; } virtual CallNode* asCallNode() { return this; };
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

    virtual void visit(AstVisitor* visitor); virtual bool isPrintNode() const { return true; } virtual PrintNode* asPrintNode() { return this; };
};



static inline const char* tokenStr(TokenKind token) {

        switch (token) {
            case tEOF: return "tEOF"; case tLPAREN: return "tLPAREN"; case tRPAREN: return "tRPAREN"; case tLBRACE: return "tLBRACE"; case tRBRACE: return "tRBRACE"; case tASSIGN: return "tASSIGN"; case tOR: return "tOR"; case tAND: return "tAND"; case tNOT: return "tNOT"; case tEQ: return "tEQ"; case tNEQ: return "tNEQ"; case tGT: return "tGT"; case tGE: return "tGE"; case tLT: return "tLT"; case tLE: return "tLE"; case tRANGE: return "tRANGE"; case tADD: return "tADD"; case tSUB: return "tSUB"; case tMUL: return "tMUL"; case tDIV: return "tDIV"; case tMOD: return "tMOD"; case tINCRSET: return "tINCRSET"; case tDECRSET: return "tDECRSET"; case tDOUBLE: return "tDOUBLE"; case tINT: return "tINT"; case tSTRING: return "tSTRING"; case tCOMMA: return "tCOMMA"; case tSEMICOLON: return "tSEMICOLON"; case tIDENT: return "tIDENT"; case tERROR: return "tERROR"; case tUNDEF: return "tUNDEF";
        default:
            return "<unknown>";

        }
}

static inline const char* tokenOp(TokenKind token) {

        switch (token) {
            case tEOF: return ""; case tLPAREN: return "("; case tRPAREN: return ")"; case tLBRACE: return "{"; case tRBRACE: return "}"; case tASSIGN: return "="; case tOR: return "||"; case tAND: return "&&"; case tNOT: return "!"; case tEQ: return "=="; case tNEQ: return "!="; case tGT: return ">"; case tGE: return ">="; case tLT: return "<"; case tLE: return "<="; case tRANGE: return ".."; case tADD: return "+"; case tSUB: return "-"; case tMUL: return "*"; case tDIV: return "/"; case tMOD: return "%"; case tINCRSET: return "+="; case tDECRSET: return "-="; case tDOUBLE: return ""; case tINT: return ""; case tSTRING: return ""; case tCOMMA: return ","; case tSEMICOLON: return ";"; case tIDENT: return ""; case tERROR: return ""; case tUNDEF: return "";
        default:
            return "???";

        }
}

static inline int tokenPrecedence(TokenKind token) {

        switch (token) {
            case tEOF: return 0; case tLPAREN: return 0; case tRPAREN: return 0; case tLBRACE: return 0; case tRBRACE: return 0; case tASSIGN: return 2; case tOR: return 4; case tAND: return 5; case tNOT: return 0; case tEQ: return 9; case tNEQ: return 9; case tGT: return 10; case tGE: return 10; case tLT: return 10; case tLE: return 10; case tRANGE: return 9; case tADD: return 12; case tSUB: return 12; case tMUL: return 13; case tDIV: return 13; case tMOD: return 13; case tINCRSET: return 14; case tDECRSET: return 14; case tDOUBLE: return 0; case tINT: return 0; case tSTRING: return 0; case tCOMMA: return 0; case tSEMICOLON: return 0; case tIDENT: return 0; case tERROR: return 0; case tUNDEF: return 0;
        default:
            return 0;

        }
}

}
