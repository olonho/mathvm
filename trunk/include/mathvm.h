#ifndef _MATHVM_H
#define _MATHVM_H

#include <stdint.h>

#include <string>
#include <cassert>
#include <vector>
#include <map>

namespace mathvm {

using namespace std;

#define FOR_BYTECODES(DO)                                               \
        DO(INVALID, "Invalid instruction.", 1)                          \
        DO(DLOAD, "Load double on TOS, inlined into insn stream.", 9)   \
        DO(ILOAD, "Load int on TOS, inlined into insn stream.", 9)      \
        DO(SLOAD, "Load string reference on TOS, next two bytes - constant id.", 3)   \
        DO(DLOAD0, "Load double 0 on TOS.", 1)                          \
        DO(ILOAD0, "Load int 0 on TOS.", 1)                             \
        DO(DLOAD1, "Load double 1 on TOS.", 1)                          \
        DO(ILOAD1, "Load int 1 on TOS.", 1)                             \
        DO(DLOADM1, "Load double -1 on TOS.", 1)                        \
        DO(ILOADM1, "Load int -1 on TOS.", 1)                           \
        DO(DADD, "Add 2 doubles on TOS, push value back.", 1)           \
        DO(IADD, "Add 2 ints on TOS, push value back.", 1)              \
        DO(DSUB, "Subtract 2 doubles on TOS (lower from upper), push value back.", 1) \
        DO(ISUB, "Subtract 2 ints on TOS (lower from upper), push value back.", 1) \
        DO(DMUL, "Multiply 2 doubles on TOS, push value back.", 1)      \
        DO(IMUL, "Multiply 2 ints on TOS, push value back.", 1)         \
        DO(DDIV, "Divide 2 doubles on TOS (upper to lower), push value back.", 1) \
        DO(IDIV, "Divide 2 ints on TOS (upper to lower), push value back.", 1) \
        DO(DNEG, "Negate double on TOS.", 1)                            \
        DO(INEG, "Negate int on TOS.", 1)                               \
        DO(IPRINT, "Pop and print integer TOS.", 1)                     \
        DO(DPRINT, "Pop and print double TOS.", 1)                      \
        DO(SPRINT, "Pop and print string TOS.", 1)                      \
        DO(I2D,  "Convert int on TOS to double.", 1)                    \
        DO(D2I,  "Convert double on TOS to int.", 1)                    \
        DO(SWAP, "Swap 2 topmost values.", 1)                           \
        DO(POP, "Remove topmost value.", 1)                             \
        DO(LOADDVAR, "Load double from variable, whose id inlined to insn stream, push on TOS.", 2) \
        DO(LOADIVAR, "Load int from variable, whose id inlined to insn stream, push on TOS.", 2) \
        DO(LOADSVAR, "Load string from variable, whose id inlined to insn stream, push on TOS.", 2) \
        DO(STOREDVAR, "Pop TOS and store to double variable, whose id inlined to insn stream.", 2) \
        DO(STOREIVAR, "Pop TOS and store to int variable, whose id inlined to insn stream.", 2) \
        DO(STORESVAR, "Pop TOS and store to string variable, whose id inlined to insn stream.", 2) \
        DO(DCMP, "Compare 2 topmost doubles, pushing libc-stryle comparator value cmp(upper, lower) as integer.", 1) \
        DO(ICMP, "Compare 2 topmost ints, pushing libc-stryle comparator value cmp(upper, lower) as integer.", 1) \
        DO(JA, "Jump always, next two bytes - signed offset of jump destination.", 3) \
        DO(IFICMPNE, "Compare two topmost integers and jump if upper != lower, next two bytes - signed offset of jump destination.", 3) \
        DO(IFICMPE, "Compare two topmost integers and jump if upper == lower, next two bytes - signed offset of jump destination.", 3) \
        DO(IFICMPG, "Compare two topmost integers and jump if upper > lower, next two bytes - signed offset of jump destination.", 3) \
        DO(IFICMPGE, "Compare two topmost integers and jump if upper >= lower, next two bytes - signed offset of jump destination.", 3) \
        DO(IFICMPL, "Compare two topmost integers and jump if upper < lower, next two bytes - signed offset of jump destination.", 3) \
        DO(IFICMPLE, "Compare two topmost integers and jump if upper <= lower, next two bytes - signed offset of jump destination.", 3) \
        DO(DUMP, "Dump value on TOS, without removing it.", 1)        \
        DO(STOP, "Stop execution.", 1)                                  \
        DO(CALL, "Call function, next two bytes - unsigned function id.", 3) \
        DO(RETURN, "Return to call location", 1) \
        DO(BREAK, "Breakpoint for the debugger.", 1)

typedef enum {
#define ENUM_ELEM(b, d, l) BC_##b,
    FOR_BYTECODES(ENUM_ELEM)
#undef ENUM_ELEM
    BC_LAST
} Instruction;

typedef enum {
    VT_INVALID = 0,
    VT_VOID,
    VT_DOUBLE,
    VT_INT,
    VT_STRING
} VarType;

typedef enum {
    VK_INTERNAL, // if variable is created by runtime.
    VK_EXTERNAL  // if variable is externally created.
} VarKind;

class Status {
    bool _ok;
    string _error;
    uint32_t _position;

  public:
    Status() :
    _ok(true),  _error(""), _position(INVALID_POSITION) {
    }

    explicit Status(const char* error, uint32_t position = INVALID_POSITION) :
        _ok(false), _error(error), _position(position) {
    }

    Status(const string& error, uint32_t position = INVALID_POSITION) :
        _ok(false), _error(error), _position(position) {
    }

    bool isOk() const {
        return _ok;
    }

    bool isError() const {
        return !_ok;
    }

    const string& getError() const {
        return _error;
    }

    const uint32_t getPosition() const {
        return _position;
    }

    static const uint32_t INVALID_POSITION = 0xffffffff;
};

class Var {
    VarType _type;
    VarKind _kind;
    string _name;
    union {
        double _doubleValue;
        int64_t _intValue;
        const char* _stringValue;
    };

  public:
    Var(VarType type, const string& name, VarKind kind = VK_EXTERNAL);

    void setDoubleValue(double value) {
        assert(_type == VT_DOUBLE);
        _doubleValue = value;
    }

    double getDoubleValue() const {
        assert(_type == VT_DOUBLE);
        return _doubleValue;
    }

    void setIntValue(int64_t value) {
        assert(_type == VT_INT);
        _intValue = value;
    }

    int64_t getIntValue() const {
        assert(_type == VT_INT);
        return _intValue;
    }

    void setStringValue(const char* value) {
        assert(_type == VT_STRING);
        _stringValue = value;
    }

    const char* getStringValue() const {
        assert(_type == VT_STRING);
        return _stringValue;
    }

    const string& name() const {
        return _name;
    }

    VarType type() const {
        return _type;
    }

    VarKind kind() const {
        return _kind;
    }

    void print();
};

class Bytecode;
class Label {
    Bytecode* _code;
    uint32_t _bci;
    vector<uint32_t> _relocations;
public:
    static const uint32_t INVALID_BCI = 0xffffffff;

    Label(Bytecode* code, uint32_t bci = INVALID_BCI) : _code(code), _bci(bci) {
    }

    ~Label() {
        assert(_relocations.size() == 0);
    }

    uint32_t bci() const {
        assert(isBound());
        return _bci;
    }

    int32_t offsetOf(uint32_t address) const {
        return bci() - address;
    }

    void bind(uint32_t address);

    bool isBound() const {
        return _bci != INVALID_BCI;
    }

    void addRelocation(uint32_t bciOfRelocation);
};

class Bytecode {
    vector<uint8_t> _data;
 public:
    void put(uint32_t index, uint8_t b) {
        if (index >= _data.size()) {
            _data.resize(index+1);
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
        return (Instruction)get(index);
    }

    void addInsn(Instruction insn) {
        add((uint8_t)insn);
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
            uint8_t bits[sizeof(T)];
        } u;
        for (uint32_t i=0; i<sizeof(u.bits); i++) {
            u.bits[i] = get(index+i);
        }
        return u.val;
    }

    template<class T> void addTyped(T d) {
        union {
            T val;
            uint8_t bits[sizeof(T)];
        } u;

        u.val = d;
        for (uint32_t i=0; i<sizeof(u.bits); i++) {
            add(u.bits[i]);
        }
    }

    template<class T> void setTyped(uint32_t index, T d) {
        union {
            T val;
            uint8_t bits[sizeof(T)];
        } u;

        u.val = d;
        for (uint32_t i=0; i<sizeof(u.bits); i++) {
            set(index+i, u.bits[i]);
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

    void dump() const;
};

class AstFunction;
class TranslatedFunction {
    AstFunction* _function;
    uint16_t _id;

public:
    TranslatedFunction(AstFunction* function) :
    _function(function), _id(0) {
    }

    virtual ~TranslatedFunction();

    const string& name() const;
    VarType returnType() const;
    VarType parameterType(uint32_t index) const;
    uint32_t parametersNumber() const;

    void assignId(uint16_t);
    uint16_t id() const { return _id; }
};

class BytecodeFunction : public TranslatedFunction {
    Bytecode _bytecode;

public:
    BytecodeFunction(AstFunction* function) :
      TranslatedFunction(function) {
    }

    Bytecode* bytecode() {
      return &_bytecode;
    }
};

class Code {
    typedef map<string, uint16_t> FunctionMap;
    typedef map<string, uint16_t> ConstantMap;

    vector<TranslatedFunction*> _functions;
    vector<string> _constants;
    FunctionMap _functionById;
    ConstantMap _constantById;
    
public:
    Code();
    virtual ~Code();

    uint16_t addFunction(TranslatedFunction* function);
    TranslatedFunction* functionById(uint16_t index) const;
    TranslatedFunction* functionByName(const string& name) const;

    uint16_t makeStringConstant(const string& str);
    const string& constantById(uint16_t id) const;

    /**
     * Execute this code with passed parameters, and update vars
     * in array with new values from topmost scope, if code says so.
     */
    virtual Status* execute(vector<Var*> vars) = 0;
};

class Translator {
  public:
    static Translator* create(const string& impl = "");

    virtual ~Translator() {}
    virtual Status* translate(const string& program, Code* *code) = 0;
};

// Utility functions.
char* loadFile(const char* file);
void positionToLineOffset(const string& text,
                          uint32_t position, uint32_t& line, uint32_t& offset);

}
#endif // _MATHVM_H
