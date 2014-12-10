#pragma once

#include <map>
#include <set>
#include <stack>
#include "../../../../../include/ast.h"
#include "../../../../../include/visitors.h"


namespace mathvm {

    class ClosureAnalyzer : public AstBaseVisitor {
        struct CapturedVariableMeta {
            CapturedVariableMeta() : isRead(false), isWritten(false) {
            }

            std::set<AstFunction const*> capturedBy;
            bool isRead;
            bool isWritten;
        };

        std::map<const AstFunction *, std::set<AstVar const*>> capturedVars;
        std::map<const AstVar *, CapturedVariableMeta> meta;
        std::map<const Scope*, const AstFunction*> scopesToFunctions;
        bool isFunctionScope(Scope* s) { return scopesToFunctions.find(s) != scopesToFunctions.end(); }
    public:

        virtual void visitLoadNode(LoadNode *node);

        virtual void visitBlockNode(BlockNode *node);

        virtual void visitStoreNode(StoreNode *node);


        bool isCaptured(AstVar const *var) {
            return meta.find(var) != meta.end();
        }

        void capture(AstVar const* var) {
            AstFunction const* f = getFunction(var);
            if (capturedVars.find(f) == capturedVars.end())
                capturedVars[f] = std::set<AstVar const*>();
            capturedVars[f].insert(var);
            meta[var].capturedBy.insert(_functionStack.top());
        }
        void captureRead(AstVar const* var) {
            meta[var].isRead = true;
            capture(var);

//            std::cerr<< var->name() << " is captured during read!" << std::endl;
//            for (auto f : meta[var].capturedBy)
//                std::cerr<< f->name() << " ";
            std::cerr<< std::endl;
        }

        void captureWrite(AstVar const *var) {
            capture(var);
            meta[var].isWritten = true;
//            std::cerr<< var->name() << " is captured during write!" << std::endl;
//            for (auto f : meta[var].capturedBy)
//                std::cerr<< f->name() << " ";
            std::cerr<< std::endl;
        }

        bool inCurrentScope(AstVar* var) {
            return var->owner() == _scope;
        }

        bool isFunctionLocal(AstVar const* var) {
            Scope* cur = _scope;
            while( true ) {
                if (cur == NULL) return false;
                if (cur == var->owner()) return true;
                if (cur != var->owner() && cur == _functionStack.top()->scope()) return false;
                cur = cur->parent();
            }
        }

        AstFunction const* getFunction(AstVar const* var) {
            Scope* cur = var->owner();
            while( true ) {
                if (cur == NULL) return NULL;
                if (isFunctionScope(cur)) return scopesToFunctions[cur];
                cur = cur->parent();
            }

        }
        void visitAstFunction(AstFunction const*);

    private:
        std::stack<AstFunction const*> _functionStack;
        Scope *_scope;

        void embraceVars(Scope* scope) {
            Scope::VarIterator it(scope);
            while(it.hasNext()){
                auto v = it.next();
                capturedVars[_functionStack.top()] = std::set<const AstVar*>();
                meta[v] = CapturedVariableMeta();
            }
        }
    };

}