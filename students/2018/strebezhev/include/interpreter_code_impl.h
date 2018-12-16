#ifndef MAIN_CODE_IMPL_H
#define MAIN_CODE_IMPL_H

#include <map>
#include <stack>
#include <string>
#include <vector>
#include <memory>
#include <iostream>

#include <ast.h>
#include <mathvm.h>
#include <visitors.h>

using namespace std;
using namespace mathvm;


struct NativeVar {
  union {
    int64_t asInt;
    double asDouble;
    const char* asStr;
  };

  NativeVar() = default;

  NativeVar(const NativeVar&) = default;

  NativeVar(double val) : asDouble(val) {};

  NativeVar(int64_t val) : asInt(val) {};

  NativeVar(const char* val) : asStr(val) {};

  void bind(Var& v) {
    VarType type = v.type();

    if (type == VT_INT) {
      asInt = v.getIntValue();
    } else if (type == VT_DOUBLE) {
      asDouble = v.getDoubleValue();
    } else {
      asStr = v.getStringValue();
    }
  }

  void bindBackTo(Var& v) {
    VarType type = v.type();
    type == VT_INT ? v.setIntValue(asInt) : type == VT_DOUBLE ? v.setDoubleValue(asDouble)
                                                              : v.setStringValue(asStr);
  }
};


struct mathvm::InterpreterCodeImpl : public mathvm::Code {
  Bytecode* bc = nullptr;
  vector<NativeVar> st;
  uint32_t ip = 0;
  NativeVar regs[4];

  stack<uint32_t> instructions;
  stack<Bytecode*> bytecodes;
  stack<BytecodeFunction*> funcs;

  vector<vector<map<uint16_t, NativeVar> > > scopeIdToVarsStack;
  map<string, uint16_t> varIdByName;


  void disassemble(ostream& out, FunctionFilter* filter) override {
    FunctionIterator funIt(this);
    while (funIt.hasNext()) {
      auto* func = (BytecodeFunction*) funIt.next();
      if (filter == nullptr || filter->matches(func))
        func->disassemble(out);
    }
  }

  /*
   * Execute this code with passed parameters, and update scopeIdToVarsStack
   * in array with new values from topmost scope, if code says so.
   */
  Status* execute(vector<Var*>& in) override {
    saveElseRestoreVars(in, true);
    enter(this->functionById(0));  // top

    try {
      while (ip < bc->length()) {
        switch (bc->getInsn(ip++)) {
          #define CASE_INS(b, d, l) case BC_##b: _##b(); break;
          FOR_BYTECODES(CASE_INS)
          #undef CASE_INS
          default:
            break;
        }
      }
    } catch (const char* errMsg) {
      // thrown in BC_INVALID
      return Status::Error(errMsg, ip);
    }

    saveElseRestoreVars(in, false);
    return Status::Ok();
  }

  void saveElseRestoreVars(vector<Var*>& vars, bool save = true) {
    for (auto var : vars) {
      auto pair = varIdByName.find(var->name());
      if (pair != varIdByName.end()) {
        auto lvar = varById(pair->second, 0);
        save ? lvar.bind(*var) : lvar.bindBackTo(*var);
      }
    }
  }

  NativeVar& varById(uint16_t varId, uint16_t scopeId) {
    return this->scopeIdToVarsStack[scopeId].back()[varId];
  }

  void enter(TranslatedFunction* function) {
    auto func = (BytecodeFunction*) function;
    instructions.push(ip);
    bytecodes.push(bc);
    funcs.push(func);

    bc = func->bytecode();
    ip = 0;

    auto& batch = this->scopeIdToVarsStack[func->scopeId()];
    batch.resize(batch.size() + 1);
  }

  void leave() {
    ip = pop(instructions);
    bc = pop(bytecodes);
    auto current = pop(funcs);

    auto& batch = this->scopeIdToVarsStack[current->scopeId()];
    batch.resize(batch.size() - 1);
  }

  int64_t callNativeFunction(const void* code,
                             const intptr_t* intArgs,
                             const double* doubleArgs,
                             double& doubleResult) const {
    int64_t intResult = 0;

    asm volatile (
    "mov %4, %%r12;"
    // prepare int args
    "mov  0(%2), %%rdi;  mov  8(%2), %%rsi;  mov 16(%2), %%rdx;"
    "mov 24(%2), %%rcx;  mov 32(%2), %%r8;   mov 40(%2), %%r9;"
    // prepare fl.point args
    "movsd  0(%3), %%xmm0;  movsd  8(%3), %%xmm1;  movsd 16(%3), %%xmm2;  movsd 24(%3), %%xmm3;"
    "movsd 32(%3), %%xmm4;  movsd 40(%3), %%xmm5;  movsd 48(%3), %%xmm6;  movsd 56(%3), %%xmm7;"
    // call native func & collect result
    "call %1; mov %%rax, %0; movsd %%xmm0, 0(%%r12);"
    : "=r" (intResult)  // output
    : "r" (code), "r" (intArgs), "r" (doubleArgs), "r" (&doubleResult)
    : "rdi", "rsi", "rdx", "rcx", "r8", "r9", "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7", "rax", "rbx", "r12"  // clobbered registers
    );

    return intResult;
  }

  NativeVar poptop() {
    assert(!st.empty());
    NativeVar res = st.back();
    st.pop_back();
    return res;
  }

  uint16_t readUInt16() {
    auto ret = bc->getUInt16(ip);
    ip += sizeof(ret);
    return ret;
  }

  int16_t readInt16() {
    auto ret = bc->getInt16(ip);
    ip += sizeof(ret);
    return ret;
  }

  int64_t readInt64() {
    auto ret = bc->getInt64(ip);
    ip += sizeof(ret);
    return ret;
  }

  double readDouble() {
    auto ret = bc->getDouble(ip);
    ip += sizeof(ret);
    return ret;
  }

  // Bytecode commands processing

  // Call function, next two bytes - unsigned function id
  void _CALL() { enter(this->functionById(readUInt16())); }

  // Call native function, next two bytes - id of the native function
  void _CALLNATIVE() {
    const string* funcName;
    const Signature* signature;
    auto code = this->nativeById(readUInt16(), &signature, &funcName);

    size_t argsCount = signature->size() - 1;  // idx=0 is return type
    size_t argPos = st.size() - argsCount, intIdx = 0, dblIdx = 0;

    intptr_t intArgs[6] = {};
    double doubleArgs[8] = {};

    // put arguments to registers
    for (size_t idx = 1; idx < signature->size(); ++idx) {
      VarType argType = (*signature)[idx].first;
      argType == VT_DOUBLE ? (doubleArgs[dblIdx++] = st[argPos++].asDouble)
                           : (intArgs[intIdx++] = (intptr_t) (st[argPos++].asInt));
    }

    for (size_t i = 1; i < signature->size(); ++i) st.pop_back();

    // call
    double dVal = 0.0;
    int64_t iVal = callNativeFunction(code, intArgs, doubleArgs, dVal);

    // cast result's type
    auto returnedType = (*signature)[0].first;

    if (returnedType == VT_DOUBLE) {
      st.emplace_back(NativeVar(dVal));
    } else if (returnedType == VT_INT) {
      st.emplace_back(NativeVar((int64_t) iVal));
    } else if (returnedType == VT_STRING) {
      st.emplace_back(NativeVar((char*) iVal));
    }
  }

  // Return to call location
  void _RETURN() { leave(); }

  // Load string reference on TOS, next two bytes - constant id
  void _SLOAD() { st.emplace_back(this->constantById(readUInt16()).c_str()); }

  // Load empty string on TOS
  void _SLOAD0() { st.emplace_back(this->constantById(0).c_str()); }


  // Load double on TOS, inlined into insn stream
  void _DLOAD() { st.emplace_back(readDouble()); }

  // Load double 0 / 1 / -1 on TOS
  void _DLOAD0() { st.emplace_back((double) 0); }

  void _DLOAD1() { st.emplace_back((double) 1); }

  void _DLOADM1() { st.emplace_back((double) -1); }


  // Load int on TOS, inlined into insn stream
  void _ILOAD() { st.emplace_back(readInt64()); }

  // Load int 0 / 1 / -1 on TOS
  void _ILOAD0() { st.emplace_back((int64_t) 0); }

  void _ILOAD1() { st.emplace_back((int64_t) 1); }

  void _ILOADM1() { st.emplace_back((int64_t) -1); }


  // Add 2 doubles on TOS, push value back
  void _DADD() {
    auto l = poptop();
    auto r = poptop();
    l.asDouble += r.asDouble;
    st.push_back(l);
  }

  // Subtract 2 doubles on TOS (lower from upper), push value back
  void _DSUB() {
    auto l = poptop();
    auto r = poptop();
    l.asDouble -= r.asDouble;
    st.push_back(l);
  }

  // Multiply 2 doubles on TOS, push value back
  void _DMUL() {
    auto l = poptop();
    auto r = poptop();
    l.asDouble *= r.asDouble;
    st.push_back(l);
  }

  // Divide 2 doubles on TOS (upper to lower), push value back
  void _DDIV() {
    auto l = poptop();
    auto r = poptop();
    l.asDouble /= r.asDouble;
    st.push_back(l);
  }

  // Negate double on TOS
  void _DNEG() {
    auto l = poptop();
    l.asDouble *= -1;
    st.push_back(l);
  }


  // Add 2 ints on TOS, push value back
  void _IADD() {
    auto l = poptop();
    auto r = poptop();
    l.asInt += r.asInt;
    st.push_back(l);
  }

  // Subtract 2 ints on TOS (lower from upper), push value back
  void _ISUB() {
    auto l = poptop();
    auto r = poptop();
    l.asInt -= r.asInt;
    st.push_back(l);
  }

  // Multiply 2 ints on TOS, push value back
  void _IMUL() {
    auto l = poptop();
    auto r = poptop();
    l.asInt *= r.asInt;
    st.push_back(l);
  }

  // Divide 2 ints on TOS (upper to lower), push value back
  void _IDIV() {
    auto l = poptop();
    auto r = poptop();
    l.asInt /= r.asInt;
    st.push_back(l);
  }

  // Modulo operation on 2 ints on TOS (upper to lower), push value back
  void _IMOD() {
    auto l = poptop();
    auto r = poptop();
    l.asInt %= r.asInt;
    st.push_back(l);
  }

  // Negate int on TOS
  void _INEG() {
    auto l = poptop();
    l.asInt *= -1;
    st.push_back(l);
  }

  // Arithmetic OR of 2 ints on TOS, push value back
  void _IAOR() {
    auto l = poptop();
    auto r = poptop();
    l.asInt |= r.asInt;
    st.push_back(l);
  }

  // Arithmetic AND of 2 ints on TOS, push value back
  void _IAAND() {
    auto l = poptop();
    auto r = poptop();
    l.asInt &= r.asInt;
    st.push_back(l);
  }

  // Arithmetic XOR of 2 ints on TOS, push value back
  void _IAXOR() {
    auto l = poptop();
    auto r = poptop();
    l.asInt ^= r.asInt;
    st.push_back(l);
  }


  // Pop and print integer TOS
  void _IPRINT() { cout << poptop().asInt; }

  // Pop and print double TOS
  void _DPRINT() { cout << poptop().asDouble; }

  // Pop and print string TOS
  void _SPRINT() { cout << poptop().asStr; }


  // Convert int on TOS to double
  void _I2D() { st.emplace_back((double) poptop().asInt); }

  // Convert double on TOS to int
  void _D2I() { st.emplace_back((int64_t) poptop().asDouble); }

  // Convert string on TOS to int
  void _S2I() { st.emplace_back((int64_t) atoi(poptop().asStr)); }


  // Swap 2 topmost values
  void _SWAP() {
    auto l = poptop();
    auto r = poptop();
    st.push_back(l);
    st.push_back(r);
  }

  // Remove topmost value
  void _POP() { st.pop_back(); }


  // Load double from variable 0, push on TOS
  void _LOADDVAR0() { st.push_back(regs[0]); }

  void _LOADDVAR1() { st.push_back(regs[1]); }

  void _LOADDVAR2() { st.push_back(regs[2]); }

  void _LOADDVAR3() { st.push_back(regs[3]); }

  void _LOADIVAR0() { st.push_back(regs[0]); }

  void _LOADIVAR1() { st.push_back(regs[1]); }

  void _LOADIVAR2() { st.push_back(regs[2]); }

  void _LOADIVAR3() { st.push_back(regs[3]); }

  void _LOADSVAR0() { st.push_back(regs[0]); }

  void _LOADSVAR1() { st.push_back(regs[1]); }

  void _LOADSVAR2() { st.push_back(regs[2]); }

  void _LOADSVAR3() { st.push_back(regs[3]); }


  // Pop TOS and store to string variable 0
  void _STOREDVAR0() { regs[0] = poptop(); }

  void _STOREDVAR1() { regs[1] = poptop(); }

  void _STOREDVAR2() { regs[2] = poptop(); }

  void _STOREDVAR3() { regs[3] = poptop(); }

  void _STOREIVAR0() { regs[0] = poptop(); }

  void _STOREIVAR1() { regs[1] = poptop(); }

  void _STOREIVAR2() { regs[2] = poptop(); }

  void _STOREIVAR3() { regs[3] = poptop(); }

  void _STORESVAR0() { regs[0] = poptop(); }

  void _STORESVAR1() { regs[1] = poptop(); }

  void _STORESVAR2() { regs[2] = poptop(); }

  void _STORESVAR3() { regs[3] = poptop(); }


  // Load double from variable, whose 2-byte is id inlined to insn stream, push on TOS
  void _LOADDVAR() { st.push_back(regs[readUInt16()]); }

  // Load int from variable, whose 2-byte id is inlined to insn stream, push on TOS
  void _LOADIVAR() { st.push_back(regs[readUInt16()]); }

  // Load string from variable, whose 2-byte id is inlined to insn stream, push on TOS
  void _LOADSVAR() { st.push_back(regs[readUInt16()]); }


  // Pop TOS and store to double variable, whose 2-byte id is inlined to insn stream
  void _STOREDVAR() { regs[readUInt16()] = poptop(); }

  // Pop TOS and store to int variable, whose 2-byte id is inlined to insn stream
  void _STOREIVAR() { regs[readUInt16()] = poptop(); }

  // Pop TOS and store to string variable, whose 2-byte id is inlined to insn stream
  void _STORESVAR() { regs[readUInt16()] = poptop(); }


  // Load double from variable, whose 2-byte context and 2-byte id inlined to insn stream, push on TOS
  void _LOADCTXDVAR() { st.push_back(varById(readUInt16(), readUInt16())); }

  // Load int from variable, whose 2-byte context and 2-byte id is inlined to insn stream, push on TOS
  void _LOADCTXIVAR() { st.push_back(varById(readUInt16(), readUInt16())); }

  // Load string from variable, whose 2-byte context and 2-byte id is inlined to insn stream, push on TOS
  void _LOADCTXSVAR() { st.push_back(varById(readUInt16(), readUInt16())); }

  // Pop TOS and store to double variable, whose 2-byte context and 2-byte id is inlined to insn stream
  void _STORECTXDVAR() { varById(readUInt16(), readUInt16()) = poptop(); }

  // Pop TOS and store to int variable, whose 2-byte context and 2-byte id is inlined to insn stream
  void _STORECTXIVAR() { varById(readUInt16(), readUInt16()) = poptop(); }

  // Pop TOS and store to string variable, whose 2-byte context and 2-byte id is inlined to insn stream
  void _STORECTXSVAR() { varById(readUInt16(), readUInt16()) = poptop(); }


  // Compare 2 topmost doubles, pushing libc-style comparator value cmp(upper, lower) as integer
  void _DCMP() {
    auto l = poptop().asDouble;
    auto r = poptop().asDouble;
    st.emplace_back((int64_t) (l > r ? 1 : l == r ? 0 : -1));
  }

  // Compare 2 topmost ints, pushing libc-style comparator value cmp(upper, lower) as integer
  void _ICMP() {
    auto l = poptop().asInt;
    auto r = poptop().asInt;
    st.emplace_back((int64_t) (l > r ? 1 : l == r ? 0 : -1));
  }


  // Jump always, next two bytes - signed offset of jump destination
  void _JA() { ip += bc->getInt16(ip); }


  // Compare two topmost integers and jump if upper != lower, next two bytes - signed offset of jump destination
  void _IFICMPNE() {
    auto l = poptop().asInt;
    auto r = poptop().asInt;
    auto tj = readInt16();
    if (l != r) ip += tj - sizeof(tj);
  }

  // Compare two topmost integers and jump if upper == lower, next two bytes - signed offset of jump destination
  void _IFICMPE() {
    auto l = poptop().asInt;
    auto r = poptop().asInt;
    auto tj = readInt16();
    if (l == r) ip += tj - sizeof(tj);
  }

  void _IFICMPG() {
    auto l = poptop().asInt;
    auto r = poptop().asInt;
    auto tj = readInt16();
    if (l > r) ip += tj - sizeof(tj);
  }

  void _IFICMPGE() {
    auto l = poptop().asInt;
    auto r = poptop().asInt;
    auto tj = readInt16();
    if (l >= r) ip += tj - sizeof(tj);
  }

  void _IFICMPL() {
    auto l = poptop().asInt;
    auto r = poptop().asInt;
    auto tj = readInt16();
    if (l < r) ip += tj - sizeof(tj);
  }

  void _IFICMPLE() {
    auto l = poptop().asInt;
    auto r = poptop().asInt;
    auto tj = readInt16();
    if (l <= r) ip += tj - sizeof(tj);
  }


  // Dump value on TOS, without removing it
  void _DUMP() {}

  // Stop execution
  void _STOP() { ip = bc->length(); }

  // Breakpoint for the debugger
  void _BREAK() { ip = bc->length(); }

  void _INVALID() {
    throw "invalid bytecode";
  }


  template<typename T>
  static T pop(std::stack<T>& container) {
    assert(!container.empty());
    T res = container.top();
    container.pop();
    return res;
  }
};

#endif //MAIN_CODE_IMPL_H
