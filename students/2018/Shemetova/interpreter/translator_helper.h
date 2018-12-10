#ifndef STATICNESTINGTREE_H
#define STATICNESTINGTREE_H
#include "../../../../include/ast.h"
//#include "../../../../include/mathvm.h"
#include <cstdint>
#include <unordered_map>

namespace mathvm {

    class ScopeHelper {
        uint16_t scopeCounter = 0;
        unordered_map<Scope*, uint16_t> scopes{};
    public:

        static bool checkVarVisibility(const AstVar* var, Scope* scope) {
            const string& name = var->name();
            while (scope != 0) {
                if (scope->lookupVariable(name, false) == var) {
                    return true;
                }
                scope = scope->parent();
            }
            return false;
        }

        /*
        static uint16_t getVarScopeId(const AstVar* var, Scope* scope) {
            uint16_t level = 0; //how many levels up from scope
            bool found = false;
            const string& name = var->name();
            while (scope != 0) {
                if (scope->lookupVariable(name, false) == var) {
                    found = true;
                    break;
                }
                level++;
                scope = scope->parent();
            }
            if (found) {
                return level;
            }
            else {
                throw std::runtime_error("No such variable named " + var->name());
            }
        }
         */
        static uint16_t getVarId(const AstVar* var) {
            Scope::VarIterator it = Scope::VarIterator(var->owner(), false);
            uint16_t id = 0;
            while (it.hasNext()) {
                if (it.next() == var) {
                    break;
                }
                id++;
            }
            return id;
        }

        uint16_t countScope() {
            scopeCounter = scopeCounter + 1;
            return scopeCounter;
        }

        /*
        uint16_t getScopeId(Scope* scope) {
            scope = scope->parent();
            if (scope) {
                uint32_t children = scope->childScopeNumber();
                uint16_t id = 0;
                for (uint32_t i = 0; i < children; ++i) {
                    if (scope->childScopeAt(i) == scope) {
                        break;
                    }
                    id++;
                }
                return id;
            }
            return 0; //main function
        }
         */

        void dumpScopes() {
            for (auto it = scopes.cbegin(); it != scopes.cend(); ++it) {
                 cout << it->first << " " << it->second << endl;
            }
        }

        void setFunctions(Code* code, Scope* scope) {
            Scope::FunctionIterator it = Scope::FunctionIterator(scope, false);
            while (it.hasNext()) {
                AstFunction* f = it.next();
                TranslatedFunction* func = new BytecodeFunction(f);
                code->addFunction(func);
                func->setScopeId(countScope());
                registerScope(f->node()->body()->scope(), func->scopeId());
            }
        }

        void registerScope(Scope* scope, uint16_t id) {
            scopes.insert(std::pair<Scope*, uint16_t>(scope, id));
        }

        void registerScope(Scope* scope) {
            scopes.insert(std::pair<Scope*, uint16_t>(scope, countScope()));
        }
        uint16_t getScopeId(Scope* scope) {
            //dumpScopes();
            //cout << "id is " << scope << endl;
            auto it = scopes.find(scope);
            assert(it != scopes.end());
            return it->second;
        }
        
        bool checkScope(Scope* scope) {
            auto it = scopes.find(scope);
            if (it != scopes.end()) {
                return true;
            }
            else {
                return false;
            }
        }
        
        
        
        static uint16_t countVariables(Scope* scope) {
            uint16_t result = 0;
            while (scope) {
                result+=scope->variablesCount();
                scope = scope->parent();
            }
            return result;
            
        }

        static void updateFunctionInfo(Code* code, FunctionNode * func) {
            TranslatedFunction* tf = code->functionByName(func->name());
            Scope* inner = func->body()->scope(); //check parameters?
            tf->setLocalsNumber(countVariables(inner));
           
        }

    };
}


#endif /* STATICNESTINGTREE_H */

