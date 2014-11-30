#ifndef BYTECODEGENERATOR_H
#define BYTECODEGENERATOR_H

#include "mathvm.h"
#include "visitors.h"
#include "typeinferencer.h"
#include "interpretercode.h"
#include "exceptions.h"


namespace mathvm {

typedef std::pair<uint16_t, uint16_t> ScopeVarId;

class FunctionTranslationContext
{
    typedef std::vector<uint16_t> VarIds;
    typedef std::map<string, VarIds> VarNameToIdsMap;

    uint16_t _scopeId;

    VarNameToIdsMap _varNameToIds;
    uint16_t _freeVarId;

    uint16_t getFreeVarId();

    public:
    FunctionTranslationContext(uint16_t scopeId);

    void registerSignature(const Signature& signature);
    void registerScope(Scope* scope); // FOR and IF ELSE scopes
    void registerScopeVars(Scope* scope);
    void unregisterSignature(const Signature& signature);
    void unregisterScope(Scope* scope); // FOR and IF ELSE scopes
    void unregisterScopeVars(Scope* scope);

    uint16_t addVar(const string& name);
    void addVarId(const string& name, uint16_t id);
    void removeVarId(const string& name);

    bool varNameExist(const string& varName);
    uint16_t getVarId(const string& varName);
    uint16_t getScopeId();
    ScopeVarId getScopeVarId(const string& varName);

};

class BytecodeGenerator : public AstVisitor
{
    void storeValueToVar(uint16_t scopeId, uint16_t varId, VarType varType);
    void loadValueFromVar(uint16_t scopeId, uint16_t varId, VarType varType);

    VarType _lastType;
    uint32_t _currentPosition;

    void refreshCurrentPosition(AstNode* node);
    uint32_t currentPosition();

    void checkNumber(VarType type);
    VarType lastInferredType();
    VarType getCommonType(VarType type1, VarType type2);

    void addCast(VarType type, VarType targetType);

    void addIntDoubleInsn(VarType type, Instruction intInsn, Instruction doubleInsn);
    void addStringIntDoubleInsn(VarType type, Instruction stringInsn, Instruction intInsn, Instruction doubleInsn);

    void addNeg(VarType type);
    void addAdd(VarType type);
    void addSub(VarType type);
    void addMul(VarType type);
    void addDiv(VarType type);
    void addMod(VarType type);

    void addCastIntToBool();
    void addCastToBool(VarType type);
    void addCompareWith0andReplace(Instruction ifInsn,
                                   Instruction trueInsn,
                                   Instruction falseInsn);
    void addCastTwoLastToBool(VarType type);

    void addNot(VarType type);
    void addCompInsn(VarType type, Instruction insn);

    void addBitOpInsn(VarType type, Instruction insn);
    void addAOR(VarType type);
    void addAAND(VarType type);
    void addAXOR(VarType type);

    void addPrint(VarType type);
    void addReturn();
    void addSwap();

    InterpreterCodeImpl* _code;

    typedef std::map<std::string, uint16_t> ConstantMap;
    ConstantMap _constantById;
    std::vector<string> _constants;
    uint16_t registerConstant(string const& constant);

    typedef std::map<string, uint16_t> FunctionNameToIdMap;
    FunctionNameToIdMap _functionIdByName;
    FunctionNameToIdMap _nativeIdByName;
    std::vector<NativeFunctionDescriptor> _natives;

    std::vector<Bytecode*> _bytecodeStack;
    Bytecode* bytecode();

    bool isNative(const string& name);
    uint16_t getNativeIdByName(const string& name);
    uint16_t getFunctionIdByName(const string& name);
    void registerScopeFunctions(Scope* scope);
    void registerNativeFunction(NativeCallNode* native);
    void registerFunction(AstFunction *astFunction);

    void collectArgs(FunctionNode* node);
    void translateScopeFunctions(Scope* scope);

    std::vector<FunctionTranslationContext*> _contexts;
    FunctionTranslationContext* addNewFunctionTranslationContext(uint16_t functionScopeId);
    void removeLastFunctionTranslationContext();
    FunctionTranslationContext* currentFunctionTranslationContext();

    ScopeVarId findScopeVarIdByName(const string& name);

    void addCallTopFunction();

#define VISITOR_FUNCTION(type, name) \
    virtual void visit##type(type* node);

    FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION

public:
    BytecodeGenerator();
    virtual ~BytecodeGenerator();

    Status* makeBytecode(AstFunction* top, InterpreterCodeImpl* *code);
};

}

#endif // BYTECODEGENERATOR_H
