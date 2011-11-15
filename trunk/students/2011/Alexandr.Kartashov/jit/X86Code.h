#pragma once

#include <deque>

#include "common.h"
#include "x86.h"

// ================================================================================

namespace mathvm {

  

  // --------------------------------------------------------------------------------

  struct Reg {
    Reg(char r) {
      _r = r;
    }

    char _r;
  };

  struct XmmReg {
    XmmReg(char r) {
      _r = r;
    }

    char _r;
  };

  struct Imm {
    Imm(uint64_t v) {
      _v = v;
    }

    Imm(int64_t v) {
      _v = *(uint64_t*)&v;
    }

    Imm(double v) {
      _v = *(uint64_t*)&v;
    }

    uint64_t _v;
  };

  struct Mem {
    Mem(char base, int32_t disp) {
      _base = base;
      _disp = disp;
    }

    char _base;
    int32_t _disp;
  };
    

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

    void op_r(uint8_t op, char r, bool opt_rex = false, char w = 1) {
      if ((opt_rex && r & EX_BITMASK) || !opt_rex) {
        add(x86_rex(0, r, w));
      } 
      add(op + (r & ~EX_BITMASK));
    }

    void x86_modrm_rm_32(char ro, char rm, int32_t disp) {
      add(x86_modrm(MOD_RM_D32, ro, rm));
      if (rm == RSP) {
        // It's what we really want when rm = RSP

        add(0x24); // SIB byte: scale = 00, index = 100(none), base = 100 (RSP)
      }
      addInt32(disp);
    }

    void add3(uint32_t v) {
      add(v & 0xFF);
      add((v >> 8) & 0xFF);
      add((v >> 16) & 0xFF);
    }

  public:
    using Bytecode::add;

    ~X86Code() {
      munmap(_x86code, _size);
    }

    void* x86code() {
      return _x86code;
    }

    // --------------------------------------------------------------------------------
    // Moves

    void mov_rr(char dst, char src) {
      op_rr(MOV_R_RM, dst, src);
    }

    void mov_rm(char dst, char rbase, int32_t disp) {
      add(x86_rex(dst, rbase, 1));
      add(MOV_R_RM);
      x86_modrm_rm_32(dst, rbase, disp);
    }

    void mov_mr(char src, char rbase, uint32_t disp) {
      add(x86_rex(src, rbase, 1));
      add(MOV_RM_R);
      x86_modrm_rm_32(src, rbase, disp);
    }

    void mov_r_imm(char r, uint64_t val) {
      op_r(MOV_R_IMM, r);
      addInt64(val);
    }

    /*
    void movlpd(char xmm, char rbase, int32_t offset) {
      add(0x66);
      addUInt16(MOVLPD);
      add(x86_modrm(MOD_RM_D32, xmm, rbase));
      addInt32(offset);
    }
    */

    void movq_xmm_r(char xmm, char r) {
      add(0x66);
      add(x86_rex(xmm, r, 1));
      addUInt16(MOVQ_XMM_RM);
      add(x86_modrm(MOD_RR, xmm, r));
    }

    void movq_xmm_m(char xmm, char rbase, int32_t disp) {
      add(0x66);
      add(x86_rex(xmm, rbase, 1));
      addUInt16(MOVQ_XMM_RM);
      x86_modrm_rm_32(xmm, rbase, disp);
    }

    void movq_r_xmm(char xmm, char r) {
      add(0x66);
      add(x86_rex(xmm, r, 1));
      addUInt16(MOVQ_RM_XMM);
      add(x86_modrm(MOD_RR, xmm, r));
    }

    void movq_m_xmm(char xmm, char rbase, int32_t disp) {
      add(0x66);
      add(x86_rex(xmm, rbase, 1));
      addUInt16(MOVQ_RM_XMM);
      x86_modrm_rm_32(xmm, rbase, disp);
    }

    void mov(const Reg& r, const Imm& imm) {
      mov_r_imm(r._r, imm._v);
    }

    void mov(const Reg& r, const Mem& mem) {
      mov_rm(r._r, mem._base, mem._disp);
    }

    void mov(const Mem& mem, const Reg& r) {
      mov_mr(r._r, mem._base, mem._disp);
    }

    void mov(const Reg& r1, const Reg& r2) {
      mov_rr(r1._r, r2._r);
    }

    void mov(const XmmReg& r, const Mem& mem) {
      movq_xmm_m(r._r, mem._base, mem._disp);
    }

    void mov(const Mem& mem, const XmmReg& r) {
      movq_m_xmm(r._r, mem._base, mem._disp);
    }

    void mov(const XmmReg& r1, const XmmReg& r2) {
      ABORT("Not implemented");
    }

    void mov(const XmmReg& r1, const Reg& r2) {
      movq_xmm_r(r1._r, r2._r);
    }

    // --------------------------------------------------------------------------------

    void add_rr(char dst, char src) {
      op_rr(ADD_R_RM, dst, src);
    }

    void add_xmm_xmm(char xmm1, char xmm2) {
      add3(ADDSD);
      add(x86_modrm(MOD_RR, xmm1, xmm2));
    }

    void add_rm_imm(char r, int32_t v) {
      op_rr(ADD_RM_IMM, 0, r);
      addInt32(v);
    }

    void add_r_imm(char r, int32_t v) {
      op_rr(ADD_RM_IMM, 0, r);
      addInt32(v);
    }

    void add(const Reg& dst, const Reg& src) {
      add_rr(dst._r, src._r);
    }

    void add(const XmmReg& dst, const XmmReg& src) {
      add_xmm_xmm(dst._r, src._r);
    }

    void add(const Reg& r, const Imm& imm) {
      ABORT("Not implemented");
    }

    void add(const Reg& r, const Mem& mem) {
      ABORT("Not implemented");
    }

    void add(const Mem& mem, const Reg& reg) {
      ABORT("Not implemented");
    }

    void add(const XmmReg& r, const Mem& mem) {
      ABORT("Not implemented");
    }

    void add(const Mem& mem, const XmmReg& r) {
      ABORT("Not implemented");
    }

    void add(const XmmReg& r1, const Reg& r2) {
      ABORT("Not implemented");
    }

    // --------------------------------------------------------------------------------

    void sub_rr(char dst, char src) {
      op_rr(SUB_R_RM, dst, src);
    }

    void sub_rm_imm(char r, int32_t v) {
      op_rr(SUB_RM_IMM, 5, r);
      addInt32(v);
    }

    void sub_r_imm(char r, int32_t v) {
      op_rr(SUB_RM_IMM, 5, r);
      addInt32(v);
    }

    void sub_xmm_xmm(char xmm1, char xmm2) {
      add3(SUBSD);      
      add(x86_modrm(MOD_RR, xmm1, xmm2));
    }

    void sub_xmm_m(char xmm, char base, int32_t offset) {
      add3(SUBSD);
      x86_modrm_rm_32(xmm, base, offset);
    }

    void sub(const Reg& dst, const Reg& src) {
      sub_rr(dst._r, src._r);
    }

    void sub(const XmmReg& dst, const XmmReg& src) {
      sub_xmm_xmm(dst._r, src._r);
    }

    void sub(const Reg& r, const Imm& imm) {
      ABORT("Not implemented");
    }

    void sub(const XmmReg& r, const Mem& mem) {
      ABORT("Not implemented");
    }

    void sub(const Mem& mem, const XmmReg& r) {
      ABORT("Not implemented");
    }

    void sub(const Reg& r, const Mem& mem) {
      ABORT("Not implemented");
    }

    void sub(const Mem& mem, const Reg& reg) {
      ABORT("Not implemented");
    }

    void sub(const XmmReg& r1, const Reg& r2) {
      ABORT("Not implemented");
    }

    // --------------------------------------------------------------------------------

    void mul_rr(char dst, char src) {
      op_rr2(IMUL_R_RM, dst, src);
    }

    void mul_xmm_xmm(char xmm1, char xmm2) {
      add3(MULSD);
      add(x86_modrm(MOD_RR, xmm1, xmm2));
    }

    void mul(const Reg& dst, const Reg& src) {
      mul_rr(dst._r, src._r);
    }

    void mul(const XmmReg& dst, const XmmReg& src) {
      mul_xmm_xmm(dst._r, src._r);
    }

    void mul(const Reg& r, const Imm& imm) {
      ABORT("Not implemented");
    }

    void mul(const XmmReg& r, const Mem& mem) {
      ABORT("Not implemented");
    }

    void mul(const Mem& mem, const XmmReg& r) {
      ABORT("Not implemented");
    }

    void mul(const Reg& r, const Mem& mem) {
      ABORT("Not implemented");
    }

    void mul(const Mem& mem, const Reg& reg) {
      ABORT("Not implemented");
    }

    void mul(const XmmReg& r1, const Reg& r2) {
      ABORT("Not implemented");
    }

    // --------------------------------------------------------------------------------

    void div_r(char r) {
      op_rr(IDIV_RM, 7, r);
    }

    void div_xmm_xmm(char xmm1, char xmm2) {
      add3(DIVSD);
      add(x86_modrm(MOD_RR, xmm1, xmm2));
    }

    void div(const Reg& dst, const Reg& src) {
      mov_rr(RAX, dst._r);

      push_r(RDX);
      push_r(RCX);
      
      mov_rr(RCX, src._r);
      xor_rr(RDX, RDX);
      div_r(RCX);
      
      pop_r(RCX);
      pop_r(RDX);

      mov_rr(dst._r, RAX);
    }

    void div(const XmmReg& dst, const XmmReg& src) {
      div_xmm_xmm(dst._r, src._r);
    }

    void div(const Reg& r, const Imm& imm) {
      ABORT("Not implemented");
    }

    void div(const XmmReg& r, const Mem& mem) {
      ABORT("Not implemented");
    }

    void div(const Mem& mem, const XmmReg& r) {
      ABORT("Not implemented");
    }

    void div(const Reg& r, const Mem& mem) {
      ABORT("Not implemented");
    }

    void div(const Mem& mem, const Reg& reg) {
      ABORT("Not implemented");
    }

    void div(const XmmReg& r1, const Reg& r2) {
      ABORT("Not implemented");
    }

    // --------------------------------------------------------------------------------

    void neg_r(char r) {
      add(x86_rex(r, 3, 1));
      add(NEG_RM);
      add(x86_modrm(MOD_RR, 3, r));
    }

    // --------------------------------------------------------------------------------

    void push_r(char r) {
      op_r(PUSH_R, r, true, 1);
    }

    void pop_r(char r) {
      op_r(POP_R, r, true, 1);
    }

    void push(uint32_t imm) {
      add(PUSH_IMM);
      addTyped(imm);
    }

    // --------------------------------------------------------------------------------

    void call_r(char r) {
      //add(x86_rex(2, r, 1));
      add(CALL_RM);
      add(x86_modrm(MOD_RR, 2, r));
    }

    void call_rel(const char* p) {
      add(CALL_REL32);
      addReloc(p);
    }

    template<typename T>
    void jcc_rel32(const NativeLabel<T>& label, char cond) {
      add(x86_cond(JCC_REL32, cond));
      NativeLabel<int32_t> l(this);
      l.bind(label);
    }

    NativeLabel<int32_t> jcc_rel32(char cond) {
      addInt16(x86_cond(JCC_REL32, cond));
      return NativeLabel<int32_t>(this);
    }

    NativeLabel<int32_t> jmp_rel32() {
      add(JMP_REL32);
      return NativeLabel<int32_t>(this);
    }

    // --------------------------------------------------------------------------------

    void setcc_r(char r, char cond) {
      add(x86_rex(0, r, 0));
      addInt16(x86_cond(SETCC_RM, cond));
      add(x86_modrm(MOD_RR, 0, r));
    }

    void cmp_rr(char r1, char r2) {
      add(x86_rex(r1, r2, 1));
      add(CMP_R_RM);
      add(x86_modrm(MOD_RR, r1, r2));      
    }

    void cmp_xmm_xmm(char r1, char r2, char pred) {
      add3(CMPSD);
      add(x86_modrm(r1, r2, 1));
      add(pred);
    }

    void cmp(const Reg& dst, const Reg& src) {
      cmp_rr(dst._r, src._r);
    }

    void cmp(const XmmReg& dst, const XmmReg& src, char op = -1) {
      cmp_xmm_xmm(dst._r, src._r, op);
    }

    void cmp(const Reg& r, const Imm& imm) {
      ABORT("Not implemented");
    }

    void cmp(const Reg& r, const Mem& mem) {
      ABORT("Not implemented");
    }

    void cmp(const Mem& mem, const Reg& reg) {
      ABORT("Not implemented");
    }

    void cmp(const XmmReg& r, const Mem& mem, char op = -1) {
      ABORT("Not implemented");
    }

    void cmp(const Mem& mem, const XmmReg& r, char op = 1) {
      ABORT("Not implemented");
    }

    void cmp(const XmmReg& r1, const Reg& r2, char op = -1) {
      ABORT("Not implemented");
    }

    // --------------------------------------------------------------------------------

    void test_rr(char r1, char r2) {
      add(x86_rex(r2, r1, 1));
      add(TEST_RM_R);
      add(x86_modrm(MOD_RR, r2, r1));
    }

    void and_r_imm8(char r, uint8_t imm) {
      add(x86_rex(4, r, 1));
      add(AND_RM_IMM);
      add(x86_modrm(MOD_RR, 4, r));
      add(imm);
    }

    void xor_rr(char dst, char src) {
      add(x86_rex(dst, src, 1));
      add(XOR_R_RM);
      add(x86_modrm(MOD_RR, dst, src));
    }

    // --------------------------------------------------------------------------------
    /* Code linking */

    void addReloc(const char* p) {
      _relocs.push_back(Reloc(p, current()));
      addTyped<int32_t>(0);
    }

    void link() {
      _size = _data.size();
      _x86code = (char*)mmap(NULL, _size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
      memcpy(_x86code, &_data[0], _size);
      _data.clear();

      for (Relocs::const_iterator it = _relocs.begin(); it != _relocs.end(); ++it) {
        *(int32_t*)(_x86code + it->offset) = (_x86code + it->offset + sizeof(int32_t)) - it->anchor;
      }

      mprotect(_x86code, _size, PROT_READ | PROT_EXEC);
    }

  private:
    struct Reloc {
      Reloc(const char* anch, uint32_t off) {
        anchor = anch;
        offset = off;
      }

      const char* anchor;
      uint32_t offset;
    };

    typedef std::deque<Reloc> Relocs;

    Relocs _relocs;
    char* _x86code;
    size_t _size;
  };

  typedef X86Code NativeCode;
}
