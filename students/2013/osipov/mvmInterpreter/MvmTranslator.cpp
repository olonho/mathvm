
#include "parser.h"
#include "MvmTranslator.h"

using namespace mathvm;

void mathvm::scopeEnter(Bytecode* nextIns, uint16_t scopeId) {
    nextIns -> addInsn(BC_ILOAD1);
    nextIns -> addInsn(BC_LOADIVAR);
    nextIns -> addUInt16(scopeId);
    nextIns -> addInsn(BC_IADD);
    nextIns -> addInsn(BC_STOREIVAR);
    nextIns -> addUInt16(scopeId);
}

void mathvm::scopeExit(Bytecode* nextIns, uint16_t scopeId) {
    nextIns -> addInsn(BC_LOADIVAR);
    nextIns -> addUInt16(scopeId);
    nextIns -> addInsn(BC_ILOADM1);
    nextIns -> addInsn(BC_IADD);
    nextIns -> addInsn(BC_STOREIVAR);
    nextIns -> addUInt16(scopeId);
}

MvmTranslator::MvmTranslator() {
}

void Val::store(Bytecode* dst) {
    switch (type) {
        case VT_STRING: dst -> addInsn(BC_STORECTXSVAR);
            break;
        case VT_DOUBLE: dst -> addInsn(BC_STORECTXDVAR);
            break;
        case VT_INT: dst -> addInsn(BC_STORECTXIVAR);
            break;
        default: throw std::runtime_error("Can't store val of this type");
    }
    dst -> addUInt16(scopeId);
    dst -> addUInt16(id);
}

void Val::load(Bytecode* dst) {
    switch (type) {
        case VT_STRING: dst -> addInsn(BC_LOADCTXSVAR);
            break;
        case VT_DOUBLE: dst -> addInsn(BC_LOADCTXDVAR);
            break;
        case VT_INT: dst -> addInsn(BC_LOADCTXIVAR);
            break;
        default: throw std::runtime_error("Can't load val of this type");
    }
    dst -> addUInt16(scopeId);
    dst -> addUInt16(id);
}

Val Val::define(VarMap& vars, uint16_t scopeId, string const& name, VarType type) {
    if (uint16_t(*vars.nextId + 1) < *vars.nextId) {
        throw std::runtime_error("Too many variables");
    }
    Val val = {(*vars.nextId)++, type, name, scopeId};
    vars.varMap[name].push_back(val);
    return val;
}

Val Val::defScope(VarMap& vars, Bytecode* nextIns) {
    if (uint16_t(*vars.nextId + 1) < *vars.nextId) {
        throw runtime_error("Too many variables");
    }
    Val scope = {(*vars.nextId)++, VT_INT, "$scope", 0};
    vars.varMap["$scope"].push_back(scope);
    nextIns -> addInsn(BC_ILOAD0);
    nextIns -> addInsn(BC_STOREIVAR);
    nextIns -> addUInt16(scope.id);
    return scope;
}

Val Val::fromScope(const VarMap& vars, const string& name) {
    VarMap::c_iterator vmIt = vars.varMap.find(name);
    if (vmIt == vars.varMap.end()) {
        throw runtime_error("Variable not found!");
    }
    for (vector<Val>::const_reverse_iterator vIt = vmIt -> second.rbegin(); vIt != vmIt -> second.rend(); ++vIt) {
        return *vIt;
    }
    throw runtime_error("Variable not found!");
}

void Val::unbound(VarMap& vars) {
    VarMap::iterator vmIt = vars.varMap.find(name);
    if (vmIt == vars.varMap.end()) {
        throw runtime_error("Can't find variable");
    }
    for (vector<Val>::iterator vIt = vmIt -> second.begin(); vIt != vmIt -> second.end(); ++vIt) {
        if (vIt -> scopeId != scopeId) {
            continue;
        }
        vmIt -> second.erase(vIt);
        return;
    }
    throw runtime_error("Can't find variable");
}

Status* MvmTranslator::translate(const string& program, Code**code) {
    Parser parser;
    if (Status * s = parser.parseProgram(program)) {
        return s;
    }

    MvmBytecode* bcode = new MvmBytecode();
    *code = bcode;
    VarMap varMap;
    FunMap funMap;
    Val scopeId = Val::defScope(varMap, bcode -> getBytecode());
    try {
        MvmTranslateVisitor().accept(scopeId.id, bcode, bcode -> getBytecode(), funMap, varMap, VT_VOID, parser.top() -> node() -> body());
        bcode -> getBytecode() -> addInsn(BC_STOP);
    } catch (std::runtime_error e) {
        return new Status(e.what());
    }
    return 0;
}

int64_t mathvm::bwOr(int64_t v1, int64_t v2) {
    return v1 | v2;
}

int64_t mathvm::bwAnd(int64_t v1, int64_t v2) {
    return v1 & v2;
}

int64_t mathvm::bwXor(int64_t v1, int64_t v2) {
    return v1 ^ v2;
}