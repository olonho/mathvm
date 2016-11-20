#ifndef PROJECT_SCOPE_STACK_H
#define PROJECT_SCOPE_STACK_H

#include "mathvm.h"
#include "ast.h"
#include "TranslatorError.h"

#include <stack>
#include <map>
#include <utility>

namespace mathvm
{

    struct VarData {
        const VarType type;
        const uint16_t context;
        const uint16_t id;
        const string& name;
        VarData(VarType type, uint16_t context, uint16_t id, const string& name)
                : type(type), context(context), id(id), name(name) {}
    };

    class TranslatorContext {
    public:
        TranslatorContext(): _cur_scope(nullptr), returnPos(-1) {
            _context_ids.push(0);
            _locals_counts.push(0);
        }

        /**
         * @param scope scope to enter (push)
         * @param functions functions defined withtn scope paired with ther context ids (function ids)
         *        so this functions names are checked during pushScope on that they are truly exists
         *        with FunctionIterator for this scope
         */
        void pushScope(Scope* scope, std::map<string, BytecodeFunction*> functions);

        void pushFunctionScope(Scope* scope, BytecodeFunction* f);

        void popScope();

        VarData getVarByName(const std::string& var_name);

        BytecodeFunction* getFunByName(const std::string& fun_name);

        uint16_t currentContextId() {
            assert(!_context_ids.empty());
            return _context_ids.top();
        }

        Bytecode* bytecode() {
            assert(!_f_bytecodes.empty());
            return _f_bytecodes.top()->bytecode();
        }

        uint16_t getLocalsNumber() {
            assert(!_locals_counts.empty());
            return _locals_counts.top();
        }

        void pushType(VarType t) {
            typeStack.push_back(t);
        }

        void popType() {
            assert(!typeStack.empty());
            typeStack.pop_back();
        }

        VarType tosType() {
            if (typeStack.empty()) {
                throw TranslatorError("Type stack is empty...no tos type available!");
            }
            return typeStack.back();
        }

        VarType prevTosType() {
            if (typeStack.size() < 2) {
                throw TranslatorError("Type stack size not big enough");
            }
            return typeStack[typeStack.size() - 2];
        }

        /**
         * @return return type of current function we are in
         */
        VarType returnType() {
            if (_f_bytecodes.empty()) {
                throw TranslatorError("No return type =(");
            }
            return _f_bytecodes.top()->returnType();
        }

        void setReturnedPos(int32_t pos) {
            returnPos = pos;
        }

        int32_t getReturnedPos() {
            return returnPos;
        }

        std::string curFunctionName() {
            if (_f_bytecodes.empty()) {
                return "";
            }
            return _f_bytecodes.top()->name();
        }

    private:
        Scope* _cur_scope;
        std::stack<BytecodeFunction*> _f_bytecodes;
        // stack with functions (context) ids, so top value is current context
        std::stack<uint16_t> _context_ids;
        // stack with number of local variables in contexts, so top is number
        // of locals in current context (i.e. local variables number within whole
        // scope of current function)
        std::stack<uint16_t> _locals_counts;
        // all variables visible from current scope
        std::map<std::string, VarData> _vars;
        std::map<std::string, BytecodeFunction*> _funs;
        // stack with types
        std::vector<VarType> typeStack;
        int32_t returnPos;


        void setCurScope(Scope* scope) {
            _cur_scope = scope;
        }

        void addVarsFromScope(Scope* scope);
        void removeVarsFromCurScope();
    };

}



#endif //PROJECT_SCOPE_STACK_H
