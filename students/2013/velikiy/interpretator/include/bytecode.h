#ifndef BYTECODE_H_
#define	BYTECODE_H_

#include "mathvm.h"
#include <vector>
#include <cassert>
using namespace std;

namespace mathvm {

    class Bytecode;

    class Label {
        Bytecode* _code;
        uint32_t _bci;
        vector<uint32_t> _relocations;
    public:
        static const uint32_t INVALID_BCI = 0xffffffff;

        Label(Bytecode* code = 0, uint32_t bci = INVALID_BCI) : _code(code), _bci(bci) {
        }

        ~Label() {
            // Shall be like that, but we can hit this path
            // if exception thrown before label bound.
            // assert(_relocations.size() == 0);
        }

        uint32_t bci() const {
            assert(isBound());
            return _bci;
        }

        int32_t offsetOf(uint32_t address) const {
            return bci() - address;
        }

        void bind(uint32_t address, Bytecode* code = 0);

        bool isBound() const {
            return _bci != INVALID_BCI;
        }

        void addRelocation(uint32_t bciOfRelocation);
    };

    class Bytecode {
    protected:
        vector<uint8_t> _data;
    public:

        void put(uint32_t index, uint8_t b) {
            if (index >= _data.size()) {
                _data.resize(index + 1);
            }
            _data[index] = b;            
        }

        void add(uint8_t b) {
            _data.push_back(b);
        }

        void addByte(uint8_t b) {
            add(b);
        }

        uint8_t get(uint32_t index) const {
            return _data[index];
        }

        void set(uint32_t index, uint8_t v) {
            _data[index] = v;
        }

        uint8_t getByte(uint32_t index) const {
            return get(index);
        }

        Instruction getInsn(uint32_t index) const {
            return (Instruction) get(index);
        }

        void addInsn(Instruction insn) {
            add((uint8_t) insn);
        }

        uint32_t current() const {
            return length();
        }

        void addBranch(Instruction insn, Label& target);

        void bind(Label& label) {
            label.bind(current());
        }

        template<class T> T getTyped(uint32_t index) const {

            union {
                T val;
                uint8_t bits[sizeof (T)];
            } u;
            for (uint32_t i = 0; i<sizeof (u.bits); i++) {
                u.bits[i] = get(index + i);
            }
            return u.val;
        }

        template<class T> void addTyped(T d) {

            union {
                T val;
                uint8_t bits[sizeof (T)];
            } u;
            u.val = d;
            for (uint32_t i = 0; i<sizeof (u.bits); i++) {
                add(u.bits[i]);
            }
        }

        template<class T> void setTyped(uint32_t index, T d) {
            
            union {
                T val;
                uint8_t bits[sizeof (T)];
            } u;

            u.val = d;
            for (uint32_t i = 0; i<sizeof (u.bits); i++) {
                set(index + i, u.bits[i]);
            }
        }

        double getDouble(uint32_t index) const {
            return getTyped<double>(index);
        }

        void addDouble(double d) {
            addTyped<double>(d);
        }

        int16_t getInt16(uint32_t index) const {
            return getTyped<int16_t>(index);
        }

        void addInt16(int16_t value) {
            addTyped<int16_t>(value);
        }

        void setInt16(uint32_t index, int16_t value) {
            setTyped<int16_t>(index, value);
        }

        uint16_t getUInt16(uint32_t index) const {
            return getTyped<uint16_t>(index);
        }

        void addUInt16(uint16_t value) {
            addTyped<uint16_t>(value);
        }

        void setUInt16(uint32_t index, uint16_t value) {
            setTyped<uint16_t>(index, value);
        }

        void addInt32(int32_t value) {
            addTyped<int32_t>(value);
        }

        int64_t getInt64(uint32_t index) const {
            return getTyped<int64_t>(index);
        }

        void addInt64(int64_t value) {
            addTyped<int64_t>(value);
        }

        uint32_t length() const {
            return _data.size();
        }

        Label currentLabel() {
            return Label(this, current());
        }

        void dump(ostream& out) const;

    };
    
}
#endif

