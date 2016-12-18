#include "AScopeData.h"

namespace mathvm {
    std::map<AstFunction *, NodeScopeData *> functionsNodeScopeData;

    ManagedVariable::ManagedVariable(const string &name, VarType type, bool isPointer) : name(name), type(type),
                                                                                         isPointer(isPointer) {}

    bool ManagedVariable::operator==(const ManagedVariable &rhs) const {
        return name == rhs.name;
    }

    bool ManagedVariable::operator!=(const ManagedVariable &rhs) const {
        return !(rhs == *this);
    }

    bool ManagedVariable::operator<(const ManagedVariable &rhs) const {
        return name < rhs.name;
    }

    bool ManagedVariable::operator>(const ManagedVariable &rhs) const {
        return rhs < *this;
    }

    bool ManagedVariable::operator<=(const ManagedVariable &rhs) const {
        return !(rhs < *this);
    }

    bool ManagedVariable::operator>=(const ManagedVariable &rhs) const {
        return !(*this < rhs);
    }

    ManagedVariable::ManagedVariable(const ManagedVariable &other) {
        name = other.name;
        type = other.type;
        isPointer = other.isPointer;
    }

    size_t total_captured_count = 0;
}