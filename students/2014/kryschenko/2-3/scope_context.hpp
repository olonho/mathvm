#ifndef CONTEXT_HPP
#define CONTEXT_HPP

#include <map>
#include <string.h>

#include "mathvm.h"

namespace mathvm {

    class VarInfo {
    public:
        uint16_t id;
        uint16_t context_id;

        VarInfo(uint16_t id, uint16_t context_id) :
                id(id),
                context_id(context_id) {

        }
    };


    class ScopeContext {

        BytecodeFunction* bf_;
        Scope* scope_;
        VarType tos_type_;
        std::map<std::string, uint16_t> vars_;
        ScopeContext* parent_;
        uint16_t id_;

    public:
        ScopeContext(BytecodeFunction* bf, Scope* scope, ScopeContext* p = NULL):
                bf_(bf),
                scope_(scope),
                parent_(p) {

            if (p != NULL) {
                id_ = p->getId() + 1;
            } else {
                id_ = 0;
            }

            Scope::VarIterator it = Scope::VarIterator(scope_);
            while(it.hasNext()) {
                AstVar *var = it.next();
                vars_[var->name()] = vars_.size() - 1;
            }

        }
        ~ScopeContext() {
        }

        bool containsVar(std::string const & var_name) {
            if (vars_.find(var_name) == vars_.end()) {
                if (parent_ == NULL) {
                    return false;
                } else {
                    return parent_->containsVar(var_name);
                }

            } else {
                return true;
            }
        }


        void addVar(AstVar const * var) {
            if (vars_.size() > 65535) {
                throw new std::runtime_error("Too much variables. Maximum count of variables: 65536");
            }
            vars_[var->name()] = vars_.size() - 1;
        }

        void setTosType(VarType t) {
            tos_type_ = t;
        }

        VarType getTosType() {
            return tos_type_;
        }

        VarInfo getVarInfo(std::string const & var_name) {
            if (vars_.find(var_name) == vars_.end()) {
                if (parent_ == NULL) {
                    throw new std::runtime_error("Var not found: " + var_name);
                } else {
                    return parent_->getVarInfo(var_name);
                }
            } else {
                return VarInfo(vars_[var_name], id_);
            }
        }

        uint16_t getId() {
            return id_;
        }


        BytecodeFunction* getBf() {
            return bf_;
        }

        uint16_t getLocalsNum() {
            return vars_.size();
        }

        ScopeContext* getParent() {
            return parent_;
        }

        Scope* getScope() {
            return scope_;
        }

    };

}

#endif