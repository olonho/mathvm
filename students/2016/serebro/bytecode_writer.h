//
// Created by andy on 11/12/16.
//

#ifndef PROJECT_BYTECODE_WRITER_H
#define PROJECT_BYTECODE_WRITER_H

#include <visitors.h>
#include <memory>
#include <unordered_map>
#include "type_deducer.h"
#include "MetaInfo.h"

namespace mathvm
{

class BytecodeWriter : public AstBaseVisitor
{
    struct VarDescription {
        VarDescription(uint16_t id, uint16_t scopeId, VarType vt) :
                id(id), scopeId(scopeId), type(vt) {}

        const uint16_t id;
        const uint16_t scopeId;
        const VarType type;
    };

    Code *_code;
    MetaInfo &_info;
    std::unordered_map<const AstVar*, VarDescription> _varDescriptions;
    uint16_t _localVariablesOffset; // equals to 0 if currently not in function scope else number of arguments
    std::vector<Scope*> _scopesStack;
    TypeDeducer _typeDeducer;

    // state
    BytecodeScope *_currentBytecodeScope;
    Bytecode* _currentCode;
    TranslatedFunction *_currentFunction;
    vector<function<void()>> startupBlockCode;
    vector<function<void()>> finishBlockCode;

    uint16_t currentScopeId() const {
        return (uint16_t) (_scopesStack.size() - 1);
    }

    void loadVar(VarDescription varDesc);
    void storeVar(VarDescription varDesc);
    void subtract(VarType type);
    void add(VarType);
    void print(VarType);
    void shortCircuitAndOr(BinaryOpNode *node);
    void comparison(BinaryOpNode *node);
    void integerBinaryOp(BinaryOpNode *node);
    void numberBinaryOp(BinaryOpNode *node);
public:
#define VISITOR_FUNCTION(type, name) \
    virtual void visit##type(type* node);

    FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION

    BytecodeWriter(Code * code, AstFunction *topLevel, MetaInfo &info)
            : _code(code), _info(info), _typeDeducer(code) {
        TranslatedFunction *translated = new BytecodeFunction{topLevel};
        _code->addFunction(translated);
        translated->setScopeId(0);
        _localVariablesOffset = 0;
        _currentFunction = translated;
        _currentBytecodeScope = nullptr;

    }

    class PositionalException : public std::logic_error {
    public:
        const uint32_t position;
        PositionalException(uint32_t position, const string &__arg) : logic_error(__arg),
        position(position) { }
    };
};
}
#endif //PROJECT_BYTECODE_WRITER_H
