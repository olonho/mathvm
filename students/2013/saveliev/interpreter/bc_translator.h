#ifndef TRANSLATOR_H
#define TRANSLATOR_H

#include <stack>

#include "ast.h"
#include "mathvm.h"
#include "parser.h"
#include "visitors.h"
#include "interpreter_code.h"

namespace mathvm {

int strlen1(const char* str);



class BytecodeTranslator: public Translator, AstVisitor {
public:
    BytecodeTranslator(): _context(0) {}

    virtual ~BytecodeTranslator();

    virtual Status* translate(const string& program, Code** codePtr);
    
private:
    struct VarInfo {
        uint16_t id;
        uint16_t ctx;
        VarInfo(uint16_t id, uint16_t ctx): id(id), ctx(ctx) {}
    };
    
    class Context {
        Context* _parent;
        BytecodeFunction* _function;
        std::map<const AstVar*, uint16_t> _varIds;
          
    public:
        Context(Context* parent, BytecodeFunction* function):
            _parent(parent), _function(function) {}
        
        BytecodeFunction* function() { return _function; }
        
        Context* parent() { return _parent; }
                
        uint16_t putVar(AstVar* var) {
            uint16_t varId = _varIds.size();
            return _varIds[var] = varId;
        }
        
        VarInfo findVar(const AstVar* var) {
            if (_varIds.find(var) != _varIds.end()) {
                return VarInfo(_varIds[var], _function->id());
                
            } else {
                assert(_parent);
                return _parent->findVar(var);
            }
        }
    };   
    Context* _context;
    InterpreterCodeImpl* _code;
    VarType _tosType;
    
    void pushContext(BytecodeFunction* function) {
        _context = new Context(_context, function);
    }
    
    void popContext() {
        Context* oldContext = _context;
        _context = oldContext->parent();
        delete oldContext;
    }    
    
    uint16_t declareVar(AstVar* var) {
        return _context->putVar(var);
    }
    
    VarInfo findVar(const AstVar* var) {
        return _context->findVar(var);
    }
    
    uint16_t currentContext() { 
        return currentFunction()->id();
    }
    
    BytecodeFunction* currentFunction() { 
        return _context->function();
    }
    
    bool currentlyTopFunc() {
        return currentContext() == 0;
    }
    
    void addCurrentLocals(size_t num) {
        assert(currentFunction());
        currentFunction()->setLocalsNumber(
                currentFunction()->localsNumber() + num);    
    }
        
    Bytecode* bc() { 
        assert(currentFunction());
        return currentFunction()->bytecode();
    }
    
    void addInsn(Instruction insn) {
//        INFO(insnName(insn));
        bc()->addInsn(insn);
    }
    
    void addBranch(Instruction insn, Label& target) {
//        INFO(insnName(insn) << " -> ");
        bc()->addBranch(insn, target);
    }    
        
    virtual void visitUnaryOpNode(UnaryOpNode* node);    
    virtual void visitBinaryOpNode(BinaryOpNode* node);        
    void logic(TokenKind op);
    void comparison(VarType type, TokenKind op);
    void arithmetics(VarType type, TokenKind op);
    void bitwise(VarType type, TokenKind op);
    VarType typeCastForLogic(VarType type);
    VarType typeCastForComparison(VarType leftType, VarType rightType);
    VarType typeCastForArithmetics(VarType leftType, VarType rightType);
    VarType typeCastForBitwise(VarType leftType, VarType rightType);
    
    virtual void visitStringLiteralNode(StringLiteralNode* node);
    virtual void visitDoubleLiteralNode(DoubleLiteralNode* node);
    virtual void visitIntLiteralNode(IntLiteralNode* node);
    
    virtual void visitLoadNode(LoadNode* node);
    virtual void visitStoreNode(StoreNode* node);
    void loading(const AstVar* var);  
    void storing(const AstVar* var);  
    
    virtual void visitForNode(ForNode* node);
    virtual void visitWhileNode(WhileNode* node);
    virtual void visitIfNode(IfNode* node);  
    
    virtual void visitBlockNode(BlockNode* node);
    virtual void visitFunctionNode(FunctionNode* node);
    void defineFunction(AstFunction* astFunc);
    void declareFunction(AstFunction* astFunc);
    
    void conversion(VarType foundType, VarType expectedType);
    bool checkTailRecCall(AstNode* node);
    void tailRecursion(CallNode* node);
    void processCallParameters(CallNode* node, TranslatedFunction* func);
    virtual void visitReturnNode(ReturnNode* node);
    virtual void visitCallNode(CallNode* node);
    virtual void visitNativeCallNode(NativeCallNode* node);
    virtual void visitPrintNode(PrintNode* node);
};

}

#endif

