#IFNDEF TRANSLATOR_IMPL_H__
#DEFINE TRANSLATOR_IMPL_H__

#include "mathvm.h"
#include "parser.h"
#include "visitors.h"

using namespace mathvm;

struct Function {
  uint16_t id;
  AstFunction* node;
};

struct AstNodeHierarchy {
    AstNode* node;
    std::vector<AstNodeHierarchy*> children;
    AstNodeHierarchy* parent;
};

class TranslatorImpl : public Translator, AstVisitor {    
    uint16_t varId;
    uint16_t scopeId;
    uint16_t funId;
    
    AstNodeHierarchy* currentHierarchy;
    AstNodeHierarchy* rootHierarchy;
    
    Code* code;
    ByteCode* byteCode;
    VarType currentType;
    uint16_t currentScope;
    void assertIntType(VarType type);
    void assertSameTypes(VarType left, VarType right);
    void assertIntTypes(VarType left, VarType right);
    void assertNumericType(VarType type);
    void assertAssignable(VarType to, VarType from);
    void pushInstruction(Instruction instruction);
    void pushCmp(VarType leftType, VarType rightType);
    VarType pushAdd(VarType leftType, VarType rightType);
    VarType pushSub(VarType leftType, VarType rightType);
    VarType pushMul(VarType leftType, VarType rightType);
    VarType pushDiv(VarType leftType, VarType rightType);    
    VarType pushUnaryMinus(VarType type);
    void storeIntoVar(AstVar* varInfo);
    void storeIntoVarById(uint16_t varId, uint16_t scopeId, VarType varType);
    void loadFromVar(AstVar* varInfo);
    void loadFromVarById(uint16_t varId, uint16_t scopeId, VarType varType);
    
    void initVisit(AstNode* node);
    void endVisit(AstNode* node);
    
    uint32_t varInfo(uint16_t varId, uint16_t scopeId);
    
    static const int translationException = 0;
    
    std::map< Scope*, std::map<String, Function >* > namesToFunctionIds;
    std::map< Scope*, AstFunction* > implToFunction;
    
    Function findFunction(std::string& name, AstNodeHierarchy* currentNode);
    
    public:
        TranslatorImpl(): varId(0), scopeId(0), funId(0), currentScope(0) {
        }
        virtual ~TranslatorImpl() {
	}
};

#ENDIF //TRANSLATOR_IMPL_H__