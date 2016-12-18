#ifndef PROJECT_SCOPE_STACK_H
#define PROJECT_SCOPE_STACK_H

#include "mathvm.h"
#include "ast.h"
#include "mathvm_error.h"
#include "bytecode_metadata..h"

#include <stack>
#include <map>
#include <utility>
#include <memory>

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
        TranslatorContext(): _cur_scope(nullptr), return_pos(-1), bc_meta(std::make_shared<BytecodeMetadata>()) {
        }

        /**
         * @param scope scope to enter (push)
         * @param functions functions defined within scope paired with flag if function is native
         *        so this functions names are checked during pushScope on that they are truly exists
         *        with FunctionIterator for this scope
         */
        void pushScope(Scope* scope, std::map<string, std::pair<BytecodeFunction*, bool>> functions);

        void pushFunctionScope(Scope* scope, BytecodeFunction* f);

        void popScope();

        VarData getVarByName(const std::string& var_name);

        std::pair<BytecodeFunction*, bool> getFunByName(const std::string& fun_name);

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
            type_stack.push_back(t);
        }

        void popType() {
            assert(!type_stack.empty());
            type_stack.pop_back();
        }

        VarType tosType() {
            if (type_stack.empty()) {
                throw TranslatorError("Type stack is empty...no tos type available!");
            }
            return type_stack.back();
        }

        VarType prevTosType() {
            if (type_stack.size() < 2) {
                throw TranslatorError("Type stack size not big enough");
            }
            return type_stack[type_stack.size() - 2];
        }

        void swapTopTosTypes() {
            if (type_stack.size() < 2) {
                throw TranslatorError("Type stack size not big enough");
            }
            auto tmp = type_stack.back();
            type_stack.back() = type_stack[type_stack.size() - 2];
            type_stack[type_stack.size() - 2] = tmp;
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
            return_pos = pos;
        }

        int32_t getReturnedPos() {
            return return_pos;
        }

        std::string curFunctionName() {
            if (_f_bytecodes.empty()) {
                return "";
            }
            return _f_bytecodes.top()->name();
        }

        std::shared_ptr<BytecodeMetadata> getBcMeta() {
            return bc_meta;
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
        // name in this map points to array -- stack of variables visible from
        // current scope, so top of the stack is used if code refers variable name
        // (top of the vars stack shadows other vars)
        std::map<std::string, std::vector<VarData>> _vars;
        std::map<std::string, std::pair<BytecodeFunction*, bool>> _funs;
        // stack with types
        std::vector<VarType> type_stack;
        int32_t return_pos;

        // bytecode metadata, which is built during working with context
        std::shared_ptr<BytecodeMetadata> bc_meta;


        void setCurScope(Scope* scope) {
            _cur_scope = scope;
        }

        void addVarsFromScope(Scope* scope);
        void removeVarsFromCurScope();
    };

}



#endif //PROJECT_SCOPE_STACK_H
