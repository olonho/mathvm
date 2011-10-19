#include <deque>
#include <stdint.h>

#include "mathvm.h"

#include "x86.h"

// ================================================================================

namespace mathvm {
  class X86Code : public Bytecode {

    void op_rr(uint8_t op, char r1, char r2) {
      add(x86_rex(r1, r2, 1));
      add(op);
      add(x86_modrm(MOD_RR, r1, r2));
    }

    void op_rr2(uint16_t op, char r1, char r2) {
      add(x86_rex(r1, r2, 1));
      addUInt16(op);
      add(x86_modrm(MOD_RR, r1, r2));
    }

    void op_r(uint8_t op, char r, bool opt_rex, char w) {
      if ((opt_rex && r & EX_BITMASK) || !opt_rex) {
        add(x86_rex(0, r, w));
      } 
      add(op + (r & ~EX_BITMASK));
    }

  public:
    void mov_rr(char r1, char r2) {
      op_rr(MOV_R_RM, r1, r2);
    }

    void mov_r_imm(char r, uint64_t val) {
      op_r(MOV_R_IMM, r, false, 1);
      addInt64(val);
    }

    void add_rr(char r1, char r2) {
      op_rr(ADD_R_RM, r1, r2);
    }

    void sub_rr(char r1, char r2) {
      op_rr(SUB_R_RM, r1, r2);
    }

    void mul_rr(char r1, char r2) {
      op_rr2(IMUL_R_RM, r1, r2);
    }

    void push_r(char r) {
      op_r(PUSH_R, r, true, 0);
    }

    void pop_r(char r) {
      op_r(POP_R, r, true, 0);
    }
  };

  typedef X86Code NativeCode;


  class NativeFunction : public TranslatedFunction {
  public:
    NativeFunction(AstFunction* node) 
      : TranslatedFunction(node) { }

    NativeCode* code() {
      return &_code;
    }

    void disassemble(std::ostream& out) const { }

  private:
    NativeCode _code;
  };


  class Runtime : public Code {
  public:
    NativeFunction* createFunction(AstFunction* fNode);

    Status* execute(std::vector<mathvm::Var*, std::allocator<Var*> >& args);

  private:
    std::deque<NativeFunction*> _functions;
  };
}
