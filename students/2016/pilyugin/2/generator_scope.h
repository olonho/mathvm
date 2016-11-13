#ifndef GENERATOR_SCOPE_H
#define GENERATOR_SCOPE_H

#include <cstdint>
#include "mathvm.h"
#include "ast.h"
#include "generator_exception.h"

namespace mathvm {

struct VarInfo {
    uint16_t varId;
    uint16_t scopeId;
};

class GeneratorScope {
public:
    GeneratorScope(BytecodeFunction* bf, Scope* scope, GeneratorScope* p = NULL) :
            bytecodeFunction_(bf),
            parent_(p) {
        scopeId_ = (p != NULL) ? p->scopeId_ + 1 : 0;

        Scope::VarIterator it = Scope::VarIterator(scope);
        while (it.hasNext()) {
            AstVar* var = it.next();
            variables_[var->name()] = variables_.size();
        }
    }

    bool containsVar(const std::string& name) {
        if (variables_.find(name) != variables_.end()) {
            return true;
        }
        return parent_ != NULL && parent_->containsVar(name);
    }

    void addVar(const AstVar* var) {
        variables_[var->name()] = variables_.size();
    }

    VarInfo getVar(const std::string& name) {
        if (variables_.find(name) != variables_.end()) {
            return {variables_[name], scopeId_};
        }
        if (parent_ == nullptr) {
            throw BytecodeGeneratorException("Variable not found: " + name);
        }
        return parent_->getVar(name);
    }

    uint16_t scopeId() {
        return scopeId_;
    }

    BytecodeFunction* bytecodeFunction() {
        return bytecodeFunction_;
    }

    uint16_t localsNumber() {
        return variables_.size();
    }

    GeneratorScope* parent() {
        return parent_;
    }

private:
    BytecodeFunction* bytecodeFunction_;
    GeneratorScope* parent_;
    std::map<std::string, uint16_t> variables_;
    uint16_t scopeId_;
};

}

#endif //GENERATOR_SCOPE_H

