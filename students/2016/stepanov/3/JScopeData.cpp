#include "JScopeData.h"

mathvm::AsmScopeVariable::AsmScopeVariable(asmjit::X86Var *var, mathvm::VarType type, bool isPointer, bool readOnly) {
    this->var = var;
    this->type = type;
    this->isPointer = isPointer;
    this->readOnly = readOnly;
}

mathvm::AsmScopeVariable::AsmScopeVariable(const mathvm::AsmScopeVariable &other) {
    var = other.var;
    type = other.type;
    isPointer = other.isPointer;
    readOnly = other.readOnly;
}

mathvm::AsmScopeVariable::AsmScopeVariable(mathvm::AsmScopeVariable &&other) {
    var = other.var;
    type = other.type;
    isPointer = other.isPointer;
    readOnly = other.readOnly;
}

mathvm::AsmScopeVariable &mathvm::AsmScopeVariable::operator=(const mathvm::AsmScopeVariable &other) {
    //проверка на самоприсваивание
    if (this == &other) {
        return *this;
    }
    var = other.var;
    type = other.type;
    isPointer = other.isPointer;
    readOnly = other.readOnly;

    return *this;
}

mathvm::AsmScopeVariable &mathvm::AsmScopeVariable::operator=(mathvm::AsmScopeVariable &&other) {
    //проверка на самоприсваивание
    if (this == &other) {
        return *this;
    }
    var = other.var;
    type = other.type;
    isPointer = other.isPointer;
    readOnly = other.readOnly;

    return *this;
}


