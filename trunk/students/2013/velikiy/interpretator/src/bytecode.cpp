#include "bytecode.h"
namespace mathvm {

    void Label::addRelocation(uint32_t bciOfRelocation) {
        _relocations.push_back(bciOfRelocation);
    }

    void Label::bind(uint32_t address, Bytecode* code) {
        assert(!isBound());
        if (code == 0) {
            code = _code;
        }
        assert(code != 0);
        _bci = address;
        for (uint32_t i = 0; i < _relocations.size(); i++) {
            uint32_t relocBci = _relocations[i];
            int32_t offset = offsetOf(relocBci);
            assert((int16_t) offset == offset);
            assert(code->getInt16(relocBci) == 0x1ead);
            code->setInt16(relocBci, offset);
        }
        _relocations.clear();
    }

    void Bytecode::dump(ostream& out) const {
        size_t len = length();
        for (size_t bci = 0; bci < len;) {
            size_t length;
            Instruction insn = getInsn(bci);
            out << bci << ": ";
            const char* name = bytecodeName(insn, &length);
            switch (insn) {
                case BC_DLOAD:
                    out << name << " " << getDouble(bci + 1);
                    break;
                case BC_ILOAD:
                    out << name << " " << getInt64(bci + 1);
                    break;
                case BC_SLOAD:
                    out << name << " @" << getUInt16(bci + 1);
                    break;
                case BC_CALL:
                case BC_CALLNATIVE:
                    out << name << " *" << getUInt16(bci + 1);
                    break;
                case BC_LOADDVAR:
                case BC_STOREDVAR:
                case BC_LOADIVAR:
                case BC_STOREIVAR:
                case BC_LOADSVAR:
                case BC_STORESVAR:
                    out << name << " @" << getUInt16(bci + 1);
                    break;
                case BC_LOADCTXDVAR:
                case BC_STORECTXDVAR:
                case BC_LOADCTXIVAR:
                case BC_STORECTXIVAR:
                case BC_LOADCTXSVAR:
                case BC_STORECTXSVAR:
                    out << name << " @" << getUInt16(bci + 1)
                            << ":" << getUInt16(bci + 3);
                    break;
                case BC_IFICMPNE:
                case BC_IFICMPE:
                case BC_IFICMPG:
                case BC_IFICMPGE:
                case BC_IFICMPL:
                case BC_IFICMPLE:
                case BC_JA:
                    out << name << " " << getInt16(bci + 1) + bci + 1;
                    break;
                default:
                    out << name;
            }
            out << endl;
            bci += length;
        }
    }

    void Bytecode::addBranch(Instruction insn, Label& target) {
        add((uint8_t) insn);
        if (target.isBound()) {
            addInt16(target.offsetOf(current()));
        } else {
            target.addRelocation(current());
            addInt16(0x1ead);
        }
    }
    
    
}