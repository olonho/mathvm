#ifndef _MATHVM_AST_PRINTER
#define _MATHVM_AST_PRINTER

#include "ast.h"
#include "InterpreterCodeImpl.h"
#include <map>
#include <stack>
#include <utility>
#include <vector>

using namespace mathvm;
using std::map;
using std::stack;
using std::pair;
using std::make_pair;

typedef enum {
    UT_LOAD,
    UT_LOAD0,
    UT_LOAD1,
    UT_LOADM1,
    UT_ADD,
    UT_SUB,
    UT_MUL,
    UT_DIV,
    UT_MOD,
    UT_NEG,
    UT_PRINT,
    UT_LOADVAR0,
    UT_LOADVAR1,
    UT_LOADVAR2,
    UT_LOADVAR3,
    UT_STOREVAR0,
    UT_STOREVAR1,
    UT_STOREVAR2,
    UT_STOREVAR3,
    UT_LOADVAR,
    UT_STOREVAR,
    UT_LOADCTXVAR,
    UT_STORECTXVAR,
    UT_CMP
} UntypedInstruction;

class BytecodeGenerationVisitor: public AstVisitor {

    stack<VarType> types;
    map< VarType, map<TokenKind, Instruction> > insnByToken;
    map< VarType, map<UntypedInstruction, Instruction> > insnByUntypedInsn;
    std::vector<AstVar*> vars;

    InterpreterCodeImpl *code;

    Bytecode* getBytecode();
    uint16_t getVarId(const string& name);
    Scope* currentScope();
    void addVar(AstVar *var);
    void removeVar();

    void removeConditionCheckingParams();
    void compareInts(Instruction insn);
    void compareDoubles(Instruction insn);

    void fillInstructionsForInt();
    void fillInstructionsForDouble();
    void fillInstructionsForString();

public:
    BytecodeGenerationVisitor(InterpreterCodeImpl *code);
    virtual ~BytecodeGenerationVisitor() {}
    
    
    virtual void visitBinaryOpNode(BinaryOpNode* node);
    virtual void visitUnaryOpNode(UnaryOpNode* node);
    virtual void visitStringLiteralNode(StringLiteralNode* node);
    virtual void visitDoubleLiteralNode(DoubleLiteralNode* node);
    virtual void visitIntLiteralNode(IntLiteralNode* node);
    virtual void visitLoadNode(LoadNode* node);
    virtual void visitStoreNode(StoreNode* node);
    virtual void visitForNode(ForNode* node);
    virtual void visitWhileNode(WhileNode* node);
    virtual void visitIfNode(IfNode* node);
    virtual void visitBlockNode(BlockNode* node);
    virtual void visitFunctionNode(FunctionNode* node);
    virtual void visitReturnNode(ReturnNode* node);
    virtual void visitCallNode(CallNode* node);
    virtual void visitNativeCallNode(NativeCallNode* node);
    virtual void visitPrintNode(PrintNode* node);
};

#endif
