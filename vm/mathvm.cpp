#include "mathvm.h"

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

}
