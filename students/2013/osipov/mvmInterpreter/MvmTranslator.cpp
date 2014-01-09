
#include "parser.h"
#include "MvmTranslator.h"

using namespace mathvm;



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

Val Val::define(VarMap& vars, uint16_t scopeId, string const& name, VarType type) {
    if (uint16_t(*vars.nextId + 1) < *vars.nextId) {
        throw std::runtime_error("Too many variables");
    }
    Val val = {++(*vars.nextId), type, name, scopeId};
    vars.varMap[name].push_back(val);
    return val;
}

void Val::unbound(VarMap& vars, Val const& v) {
    VarMap::iterator vmIt = vars.varMap.find(v.name);
    if (vmIt == vars.varMap.end()) {
        throw std::runtime_error("Can't find variable");
    }
    for (vector<Val>::iterator vIt = vmIt -> second.begin(); vIt != vmIt -> second.end(); ++vIt) {
        if (vIt -> scopeId == v.scopeId) vmIt -> second.erase(vIt);
        return;
    }
    throw std::runtime_error("Can't find variable");
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
    Val scopeId = Val::define(varMap, 0, "$scopeId", VT_INT);
    try {
        MvmTranslateVisitor().accept(scopeId.id, bcode, bcode -> getBytecode(), funMap, varMap, VT_VOID, parser.top() -> node() -> body());
    } catch (std::exception e) {
        return new Status(e.what());
    }
    return 0;
}
