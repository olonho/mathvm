#include "helpers.h"
#include "translation_exception.h"

namespace mathvm {

using std :: map;
using std :: string;
using std :: make_pair;

namespace details {

TranslatorVar :: TranslatorVar(uint16_t id, uint16_t contextId)
    : _id(id)
    , _contextId(contextId) {}

uint16_t TranslatorVar :: id() {
    return _id;
}

uint16_t TranslatorVar :: contextId() {
    return _contextId;
}

ScopeData :: ScopeData(BytecodeFunction* function, ScopeData* parent)
    : _function(function)
    , _parent(parent)
    , _variables()
    , _localsCount(0) {}

uint16_t ScopeData :: getId() {
    return _function->id();
}

uint16_t ScopeData :: getVarCount() {
    return _variables.size();
}

void ScopeData :: addAstVar(const AstVar* var) {
    ++_localsCount;
    _variables.insert(make_pair(var->name(), _variables.size()));
}

TranslatorVar ScopeData :: getVar(const AstVar* var) {
    if (_variables.find(var->name()) == _variables.end()) {
        if (_parent) {
            return _parent->getVar(var);
        } else {
            throw std :: runtime_error("Variable is not found");
        }
    } else {
        return TranslatorVar(_variables[var->name()], _function->id());
    }
}

BytecodeFunction* ScopeData :: function() {
    return _function;
}

ScopeData* ScopeData :: parent() {
    return _parent;
}

map<string, uint16_t> ScopeData :: variables() {
    return _variables;
}

int ScopeData :: localsCount() {
    return _localsCount;
}

StackValue :: StackValue() { }

StackValue StackValue :: saveInt(int64_t val) {
    StackValue ret;
    ret._holder._intVal = val;
    return ret;
}

StackValue StackValue :: saveDouble(double val) {
    StackValue ret;
    ret._holder._doubleVal = val;
    return ret;
}

StackValue StackValue :: saveStringRef(uint16_t val) {
    StackValue ret;
    ret._holder._stringRef = val;
    return ret;
}

int64_t StackValue :: intVal() {
    return _holder._intVal;
}

double StackValue :: doubleVal() {
    return _holder._doubleVal;
}

uint16_t StackValue :: stringRef() {
    return _holder._stringRef;
}

Context :: Context(BytecodeFunction* function, Context* parent)
    : _env(function->localsNumber())
    , _function(function)
    , _parent(parent)
    , _ip(0) { }

StackValue Context :: getContextVal() {
    uint16_t varId = getShort();
    uint16_t contextId = getShort();

    return getContextValHelper(varId, contextId);
}

void Context :: setContextVal(StackValue val) {
    uint16_t varId = getShort();
    uint16_t contextId = getShort();

    setContextValHelper(varId, contextId, val);
}

StackValue Context :: getVal() {
    uint16_t varId = getShort();

    return _env[varId];
}

void Context :: setVal(StackValue val) {
    uint16_t varId = getShort();

    _env[varId] = val;
}

void Context :: jump(int16_t offset) {
    _ip += offset;
}

uint16_t Context :: getStringRef() {
    uint16_t res = _function->bytecode()->getUInt16(_ip);
    _ip += sizeof(uint16_t);
    return res;
}

int64_t Context :: getInt() {
    int64_t res = _function->bytecode()->getInt64(_ip);
    _ip += sizeof(uint64_t);
    return res;
}

int16_t Context :: getShort() {
    uint16_t res = _function->bytecode()->getInt16(_ip);
    _ip += sizeof(int16_t);
    return res;
}

double Context :: getDouble() {
    double res = _function->bytecode()->getDouble(_ip);
    _ip += sizeof(double);
    return res;
}

Context* Context :: parent() {
    return _parent;
}

Instruction Context :: getCurrentInstruction() {
    return _function->bytecode()->getInsn(_ip++);
}

Bytecode* Context :: getCurrentBytecode() {
    return _function->bytecode();
}

StackValue Context :: getContextValHelper(uint16_t varId, uint16_t contextId) {
    if (contextId == _function->id()) {
        return _env[varId];
    } else {
        if (_parent == nullptr) {
            throw TranslationException("Missing variable definition");
        } else {
            return _parent->getContextValHelper(varId, contextId);
        }
    }
}

void Context :: setContextValHelper(uint16_t varId, uint16_t contextId, StackValue val) {
    if (contextId == _function->id()) {
        _env[varId] = val;
    } else {
        if (_parent == nullptr) {
            throw TranslationException("Missing variable definition");
        } else {
            _parent->setContextValHelper(varId, contextId, val);
        }
    }
}

Instruction ADD(VarType type) {
    switch (type) {
        case VT_INT : return BC_IADD;
        case VT_DOUBLE : return BC_DADD;
        default : throw TranslationException("Can't add these types");
    }
}

Instruction SUB(VarType type) {
    switch (type) {
        case VT_INT : return BC_ISUB;
        case VT_DOUBLE : return BC_DSUB;
        default : throw TranslationException("Can't subtract these types");
    }
}

Instruction MUL(VarType type) {
    switch (type) {
        case VT_INT : return BC_IMUL;
        case VT_DOUBLE : return BC_DMUL;
        default : throw TranslationException("Can't multiply these types");
    }
}

Instruction DIV(VarType type) {
    switch (type) {
        case VT_INT : return BC_IDIV;
        case VT_DOUBLE : return BC_DDIV;
        default : throw TranslationException("Can't divide these types");
    }
}

Instruction COMPARE(VarType type) {
    switch (type) {
        case VT_INT : return BC_ICMP;
        case VT_DOUBLE : return BC_DCMP;
        default : throw TranslationException("Can't compare these types");
    }
}

Instruction NEGATE(VarType type) {
    switch (type) {
        case VT_INT : return BC_INEG;
        case VT_DOUBLE : return BC_DNEG;
        default : throw TranslationException("Can't negate this type");
    }
}

Instruction LOAD(VarType type) {
    switch (type) {
        case VT_INT : return BC_LOADIVAR;
        case VT_DOUBLE : return BC_LOADDVAR;
        case VT_STRING : return BC_LOADSVAR;
        default : throw TranslationException("Can't load this type");
    }
}

Instruction STORE(VarType type) {
    switch (type) {
        case VT_INT : return BC_STOREIVAR;
        case VT_DOUBLE : return BC_STOREDVAR;
        case VT_STRING : return BC_STORESVAR;
        default : throw TranslationException("Can't store this type");
    }
}

Instruction LOAD_CTX(VarType type) {
    switch (type) {
        case VT_INT : return BC_LOADCTXIVAR;
        case VT_DOUBLE : return BC_LOADCTXDVAR;
        case VT_STRING : return BC_LOADCTXSVAR;
        default : throw TranslationException("Can't load this type");
    }
}

Instruction STORE_CTX(VarType type) {
    switch (type) {
        case VT_INT : return BC_STORECTXIVAR;
        case VT_DOUBLE : return BC_STORECTXDVAR;
        case VT_STRING : return BC_STORECTXSVAR;
        default : throw TranslationException("Can't store this type");
    }
}

Instruction PRINT(VarType type) {
    switch (type) {
        case VT_INT : return BC_IPRINT;
        case VT_DOUBLE : return BC_DPRINT;
        case VT_STRING : return BC_SPRINT;
        default : throw TranslationException("Can't print this type");
    }
}

} // end namespace details
} // end namespace mathvm
