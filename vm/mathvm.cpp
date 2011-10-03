#include "mathvm.h"
#include "ast.h"

#include <iostream>

using namespace std;

namespace mathvm {

void Label::addRelocation(uint32_t bciOfRelocation) {
    _relocations.push_back(bciOfRelocation);
}

void Label::bind(uint32_t address) {
    assert(!isBound());
    _bci = address;
    for (uint32_t i = 0; i < _relocations.size(); i++) {
        uint32_t relocBci = _relocations[i];
        int32_t offset = offsetOf(relocBci);
        assert((int16_t)offset == offset);
        assert(_code->getInt16(relocBci) == 0x1ead);
        _code->setInt16(relocBci, offset);
    }
    _relocations.clear();
}

static const char* bcName(Instruction insn, size_t& length) {
    static const struct {
        const char* name;
        Instruction insn;
        size_t length;
    } names[] = {
#define BC_NAME(b, d, l) {#b, BC_##b, l},
        FOR_BYTECODES(BC_NAME)
    };

    if (insn >= BC_INVALID && insn < BC_LAST) {
        length = names[insn].length;
        return names[insn].name;
    }

    assert(false);
    return 0;
}

void Bytecode::dump() const {
    for (size_t bci = 0; bci < length();) {
        size_t length;
        Instruction insn = getInsn(bci);
        cout << bci << ": ";
        const char* name = bcName(insn, length);
        switch (insn) {
            case BC_DLOAD:
                cout << name << " " << getDouble(bci + 1);
                break;
            case BC_ILOAD:
                cout << name << " " << getInt64(bci + 1);
                break;
            case BC_SLOAD:
                cout << name << " @" << getInt16(bci + 1);
                break;
            case BC_CALL:
            case BC_LOADDVAR:
            case BC_STOREDVAR:
            case BC_LOADIVAR:
            case BC_STOREIVAR:
            case BC_LOADSVAR:
            case BC_STORESVAR:
                cout << name << " @" << getInt16(bci + 1);
                break;
            case BC_IFICMPNE:
            case BC_IFICMPE:
            case BC_IFICMPG:
            case BC_IFICMPGE:
            case BC_IFICMPL:
            case BC_IFICMPLE:
            case BC_JA:
              cout << name << " " << getInt16(bci + 1) + bci + 1;
              break;
          default:
                cout << name;
        }
        cout << endl;
        bci += length;
    }
}

void Bytecode::addBranch(Instruction insn, Label& target) {
    add((uint8_t)insn);
    if (target.isBound()) {
        addInt16(target.offsetOf(current()));
    } else {
        target.addRelocation(current());
        addInt16(0x1ead);
    }
}

Var::Var(VarType type, const string& name, VarKind kind) :
    _type(type), _kind(kind) {
    _name = string(name);
    switch (type) {
    case VT_DOUBLE:
        setDoubleValue(0.0);
        break;
    case VT_INT:
        setIntValue(0);
        break;
    case VT_STRING:
        setStringValue(0);
        break;
    default:
        assert(false);
    }
}

void Var::print() {
    switch (_type) {
    case VT_DOUBLE:
        cout << getDoubleValue();
        break;
    case VT_INT:
        cout << getIntValue();
        break;
    case VT_STRING:
        cout << getStringValue();
        break;
        default:
            assert(false);
    }
}

Code::Code() {
    _constants.push_back("");
}

Code::~Code() {
    for (uint32_t i = 0; i < _functions.size(); i++) {
        delete _functions[i];
    }
}

uint16_t Code::addFunction(TranslatedFunction* function) {
    uint16_t id = _functions.size();
    _functions.push_back(function);
    _functionById[function->name()] = id;
    function->assignId(id);
    return id;
}

TranslatedFunction* Code::functionById(uint16_t id) const {
    if (id >= _functions.size()) {
        return 0;
    }
    return _functions[id];
}

TranslatedFunction* Code::functionByName(const string& name) const {
    FunctionMap::const_iterator it = _functionById.find(name);
    if (it == _functionById.end()) {
        return 0;
    }
    return functionById((*it).second);
}

uint16_t Code::makeStringConstant(const string& str) {
    ConstantMap::iterator it = _constantById.find(str);
    if (it != _constantById.end()) {
        return (*it).second;
    }
    uint16_t id = _constants.size();
    _constantById[str] = id;
    _constants.push_back(str);
    return id;
}

const string& Code::constantById(uint16_t id) const {
    if (id >= _constants.size()) {
        return _constants[0];
    }
    return _constants[id];
}


TranslatedFunction::~TranslatedFunction() {
  if (_function) {
    delete _function;
  }
}

const string& TranslatedFunction::name() const {
    return _function->name();
}

VarType TranslatedFunction::returnType() const {
    return _function->returnType();
}

VarType TranslatedFunction::parameterType(uint32_t index) const {
    return _function->parameterType(index);
}

uint32_t TranslatedFunction::parametersNumber() const {
    return _function->parametersNumber();
}

void TranslatedFunction::assignId(uint16_t id) {
    assert(_id == 0);
    assert(_function->isTop() || id != 0);

    _id = id;
}

}
