#ifndef INC_1_INTERPRETERCODE_H
#define INC_1_INTERPRETERCODE_H

#include <stack>
#include "mathvm.h"
#include "bytecode_translator.h"

namespace mathvm {
    class InterpreterCode : public Code {
    private:
        stack<Var> dataStack;
        stack<pair<Bytecode*, uint32_t>> insnStack;
        map<uint16_t, stack<map<uint16_t, Var*>*>>* curCopiesOfScopes = new map<uint16_t, stack<map<uint16_t, Var*>*>>();
        map<uint16_t, vector<uint16_t>>* _functionsScopesChilds;
        map<uint16_t, map<uint16_t, Var*>>* _varsByScopes;
        map<uint16_t, Var*>* _valuesByIds;
        map<const string, uint16_t>* _varsFromTopmostScope;
        void executeCurBytecode(uint16_t scopeId);
        #define INSTRUCTION_EXECUTION(b, d, l) void execute##b();
                FOR_BYTECODES(INSTRUCTION_EXECUTION)
        #undef INSTRUCTION_EXECUTION

        void storeVar(int varId, VarType varType);
        void storeVar(VarType varType);
        void storeVarByCtx(VarType varType);
        void copyScope(uint16_t scopeId);
        void finalizeFuncExecution(uint16_t scopeId);
    public:
        InterpreterCode(map<uint16_t, Var*>* varsIds,
                        map<uint16_t, map<uint16_t, Var*>>* varsByScopes,
        map<const string, uint16_t>* varsFromTopmostScope,
        map<uint16_t, vector<uint16_t>>* functionsScopesChilds) {
            _varsByScopes = varsByScopes;
            _valuesByIds = varsIds;
            _varsFromTopmostScope = varsFromTopmostScope;
            _functionsScopesChilds = functionsScopesChilds;
        }

        virtual ~InterpreterCode() {
            for (auto mapElem : *_varsByScopes) {
                for (auto elem : mapElem.second) {
                    delete(elem.second);
                }
            }
            delete(_varsByScopes);
            delete(_valuesByIds);
            delete(_varsFromTopmostScope);
            delete(_functionsScopesChilds);
            delete(curCopiesOfScopes);
        }
        virtual Status* execute(vector<Var*>& vars) override;
    };
}

#endif //INC_1_INTERPRETERCODE_H
