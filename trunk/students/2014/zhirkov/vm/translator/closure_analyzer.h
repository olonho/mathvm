#pragma once

#include <map>
#include <set>
#include <stack>
#include "../../../../../include/ast.h"
#include "../../../../../include/visitors.h"
#include "../ast_analyzer.h"


namespace mathvm {


    struct ClosureInfo {
        struct Variable {
            Variable(AstFunction const *const owner) : owner(owner), isRead(false), isWritten(false) {
            }

            AstFunction const *const owner;
            std::set<AstFunction const *> capturedBy;
            bool isRead;
            bool isWritten;
        };

        struct Function {

            std::set<AstVar const *> capturedLocals;
            std::set<const AstVar *> capturedOuterVars;

            Function(AstFunction const *const parent) : parent(parent) {
            }

            const AstFunction *const parent;
        };

        std::map<const AstVar *, Variable> vars;
        std::map<const Scope *, const AstFunction *> scopesToFunctions;
        std::map<const AstFunction *, Function*> functions;

        bool captured(AstVar const *const v) const {
            return vars.find(v) != vars.end();
        }

        bool capturedWritten(AstVar const *const v) const {
            return captured(v) && vars.at(v).isWritten;;
        }

        bool capturedRead(AstVar const *const v) const {
            return captured(v) && vars.at(v).isRead;
        }

        bool captures(AstFunction const *const f) const {
            return functions.find(f) != functions.end() && !functions.at(f)->capturedOuterVars.empty();
        }


//        void vivify(AstFunction const* const f) {
//            if (functions.find(f) == functions.end()) functions[f] = Function() ;
//        }
        virtual ~ClosureInfo() {
            for (auto item : functions) {
                delete item.second;
            }
        }
    };

    class ClosureAnalyzer : public AstAnalyzer<ClosureInfo, AstAnalyzerContext> {

    public:
        virtual void declareFunction(AstFunction const *fun);

        ClosureAnalyzer(AstFunction const *top, ostream &debugStream) : AstAnalyzer(top, "closures", debugStream) {
        }

    private:
        bool isFunctionScope(Scope *s) const {
            return _result->scopesToFunctions.find(s) != _result->scopesToFunctions.end();
        }

    public:

        virtual void visitLoadNode(LoadNode *node);

        virtual void visitBlockNode(BlockNode *node);

        virtual void visitStoreNode(StoreNode *node);

        void capture(AstVar const *var) {
            AstFunction const *owner = getOwnerFunction(var);
            _result->functions.at(owner)->capturedLocals.insert(var);

            for (AstFunction const *current = _functionStack.top();
                 current != NULL && current != owner;
                 current = _result->functions[current]->parent) {
                _result->functions.at(current)->capturedOuterVars.insert(var);
                _result->vars.at(var).capturedBy.insert(current);
            }
        }

        void captureRead(AstVar const *var) {
            _result->vars.at(var).isRead = true;
            capture(var);
        }

        void captureWrite(AstVar const *var) {
            capture(var);
            _result->vars.at(var).isWritten = true;
        }

        bool isFunctionLocal(AstVar const *var) const {
            for (Scope *cur = var->owner(); cur != NULL; cur = cur->parent())
                if (isFunctionScope(cur)) return _result->scopesToFunctions[cur] == _functionStack.top();
            return false;
        }

        AstFunction const *getOwnerFunction(AstVar const *var) const {
            Scope *cur = var->owner();
            while (true) {
                if (cur == NULL) return NULL;
                if (isFunctionScope(cur)) return _result->scopesToFunctions.at(cur);
                cur = cur->parent();
            }

        }

        void visitAstFunction(AstFunction const *);

        void debug();

    private:
        const std::set<AstVar const *> NOVARS;

        std::stack<AstFunction const *> _functionStack;
        Scope *_scope;

        void embraceVars(Scope *scope) {
            Scope::VarIterator it(scope);
            while (it.hasNext()) {
                auto v = it.next();
                _result->vars.insert(make_pair(v, ClosureInfo::Variable(_functionStack.top())));
            }
        }

    };

}