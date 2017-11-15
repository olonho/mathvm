#ifndef TRANSLATOR_UTIL_H
#define TRANSLATOR_UTIL_H

#include "mathvm.h"
#include "ast.h"
#include <vector>
#include <unordered_map>

namespace mathvm {
    namespace utils {

        struct Context {
            Context(BytecodeFunction* func, Context* parent = nullptr);
            BytecodeFunction* function();
            std::pair<uint16_t, uint16_t> locAndCtxId(const std::string& name);
            void addVar(const std::string& name);
            uint16_t id() const;
            uint16_t localsCount() const;
            Context* parent();

        private:
            BytecodeFunction* _function;
            Context* _parent;
            uint16_t _curId;
            std::unordered_map<std::string, uint16_t> _varName2Id;
        };

        VarType resultType(VarType left, VarType right);
        std::vector<Instruction> insnConvert(VarType from, VarType to);
        Instruction localScopeStoreInsn(VarType type, uint16_t localId);
        Instruction outerScopeStoreInsn(VarType type);
        Instruction localScopeLoadInsn(VarType type, uint16_t localId);
        Instruction outerScopeLoadInsn(VarType type);
        Instruction arithmeticInsn(VarType type, TokenKind kind);
        Instruction bitwiseInsn(TokenKind kind);
        Instruction comparisonInsn(TokenKind kind);
    }
}

#endif // TRANSLATOR_UTIL_H
