#pragma once

#include "includes.h"

#include <sstream>
#include <map>

namespace detail {
    inline mathvm::VarType getCommonType(mathvm::VarType t1, mathvm::VarType t2) {
        static std::map<std::pair<mathvm::VarType, mathvm::VarType>, mathvm::VarType> commonType = {
                {{mathvm::VT_INT, mathvm::VT_DOUBLE}, mathvm::VT_DOUBLE},
                {{mathvm::VT_INT, mathvm::VT_INT}, mathvm::VT_INT},
                {{mathvm::VT_INT, mathvm::VT_STRING}, mathvm::VT_INT},
                {{mathvm::VT_DOUBLE, mathvm::VT_DOUBLE}, mathvm::VT_DOUBLE},
                {{mathvm::VT_STRING, mathvm::VT_STRING}, mathvm::VT_STRING},
                
        };
        auto it = commonType.find({t1, t2});
        if (it != commonType.end()) {
            return it->second;
        }
        it = commonType.find({t2, t1});
        if (it != commonType.end()) {
            return it->second;
        }
        std::cout << "TRANSLATOR ERROR: INCOMPATIBLE TYPES:" << t1 << "," << t2 << std::endl;
        exit(42);
    }
}

struct BytecodeTranslatorVisitor : mathvm::AstBaseVisitor {

#define VISITOR_FUNCTION(type, name) \
    virtual void visit##type(mathvm::type* node) override;

    FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION

    mathvm::Bytecode getBytecode() const {
        return bytecode;
    };

    std::vector<std::string> getStringConstants() const {
        return stringConstants;
    };

    std::map<uint16_t, uint32_t> getFunctionOffsetsMap() const {
        return functionOffsetsMap;
    };

    std::map<uint16_t, std::pair<std::string, std::vector<mathvm::VarType>>> getNativeFunctions() const {
        return nativeFunctionMap;
    };

    std::map<std::string, int> getTopMostVars() {
        std::map<std::string, int> result;
        for (auto p : scopes[0].vars) {
            if (p.second < topMostVariablesNum) {
                result[p.first] = p.second;
            }
        }
        return result;
    };

private:

    struct scope {
        scope(uint16_t function_id) : function_id(function_id) {}

        std::map<std::string, uint16_t> vars;
        const uint16_t function_id;
    };

    mathvm::NativeCallNode * checkNative(mathvm::FunctionNode *node);
    std::pair<uint16_t, uint16_t> findVar(const std::string & varName);
    void generateStoreVarBytecode(const std::string & name, mathvm::VarType type);
    void generateLoadVarBytecode(const std::string & name, mathvm::VarType type);
    void generateVarOperationBytecode(const std::string & name, mathvm::Instruction ctxInsn);
    void consumeTOS(mathvm::VarType type);
    void castTOSPair(mathvm::VarType top, mathvm::VarType bottom, mathvm::VarType target);
    template <typename T>
    void handleArithmeticOperation(mathvm::BinaryOpNode* node, T functor) {
        node->right()->visit(this);
        mathvm::VarType typeRight = typeStack.back();
        node->left()->visit(this);
        mathvm::VarType typeLeft = typeStack.back();
        mathvm::VarType commonType = detail::getCommonType(typeLeft, typeRight);
        castTOSPair(typeLeft, typeRight, commonType);
        bytecode.addInsn(functor(commonType));
        typeStack.pop_back();
        typeStack.pop_back();
        typeStack.push_back(commonType);
    };

    void generateNot();
    void generateLE(mathvm::AstNode * left, mathvm::AstNode * right);
    void generateLT(mathvm::AstNode * left, mathvm::AstNode * right);


    std::vector<scope> scopes = {scope{0}};

    std::map<std::string, uint16_t> functionMap;
    std::map<uint16_t, std::pair<std::string, std::vector<mathvm::VarType>>> nativeFunctionMap;
    std::map<uint16_t, std::vector<mathvm::VarType>> functionSignatures;
    std::map<uint16_t, uint32_t> functionOffsetsMap;
    uint16_t globalFunctionCounter = 0;

    mathvm::Bytecode bytecode;
    std::vector<mathvm::VarType> typeStack;
    int topMostVariablesNum = -1;
    std::vector<std::string> stringConstants;
};