
#include "parser.h"
#include "MvmTranslator.h"

using namespace mathvm;

MvmTranslator::MvmTranslator() {
}

Val Val::define(AstNode* scope, VarMap& vars, uint16_t scopeId, string const& name, VarType type) {
    if (uint16_t(*vars.nextId + 1) < *vars.nextId) {
        throw "Too many variables";
    }
    Val val = {++(*vars.nextId), type, name,  scopeId};
    vars.varMap[name].push_back(val);
    return val;
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
    Val scopeId = Val::define(parser.top() -> node() -> body(), varMap, 0, "$scopeId", VT_INT);
    try {
        //visitor
    } catch (std::exception e) {
        return new Status(e.what());
    }
    return 0;
}
