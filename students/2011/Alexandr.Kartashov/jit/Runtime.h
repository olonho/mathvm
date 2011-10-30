#pragma once

#include <cstdio>
#include <stdint.h>

#include <deque>

#include <sys/mman.h>
#include <string.h>

#include "mathvm.h"
#include "x86.h"

// ================================================================================

namespace mathvm {
  template<typename T>
  class NativeLabel {
  public:    
    NativeLabel() { }

    NativeLabel(Bytecode* bc) {
      _label = bc->current();
      bc->addTyped<T>(0);
      _bc = bc;
    }

    void bind(uint32_t offset) {
      int32_t d = (int32_t)offset - (int32_t)_label - sizeof(T);
      _bc->setTyped<int32_t>(_label, d);
    }

    void bind(Bytecode* bc) {
      bind(bc->current());
    }

    void bind(const NativeLabel& label) {
      int32_t d = (int32_t)label._label - (int32_t)_label - sizeof(T);
      _bc->setTyped<int32_t>(_label, d);
    }

    void bind() {
      bind(_bc);
    }

    uint32_t offset() const {
      return _label;
    }

  private:
    uint32_t _label;
    Bytecode* _bc;
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
        // It's what really intended when rm = RSP

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
    ~X86Code() {
      munmap(_x86code, _size);
    }

    void done() {
      _size = _data.size();
      _x86code = mmap(NULL, _size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
      //perror("Error");
      memcpy(_x86code, &_data[0], _size);
      mprotect(_x86code, _size, PROT_READ | PROT_EXEC);
      _data.clear();
    }

    void* x86code() {
      return _x86code;
    }

    void mov_rr(char dst, char src) {
      op_rr(MOV_R_RM, dst, src);
    }

    void mov_rm(char dst, char base, int32_t offset) {
      add(x86_rex(dst, base, 1));
      add(MOV_R_RM);
      add(x86_modrm(MOD_RM_D32, dst, base));
      addInt32(offset);
    }

    void mov_mr(char src, char base, uint32_t offset) {
      add(x86_rex(src, base, 1));
      add(MOV_RM_R);
      add(x86_modrm(MOD_RM_D32, src, base));
      addInt32(offset);
    }

    void mov_r_imm(char r, uint64_t val) {
      op_r(MOV_R_IMM, r);
      addInt64(val);
    }

    void add_rr(char dst, char src) {
      op_rr(ADD_R_RM, dst, src);
    }

    void sub_rr(char dst, char src) {
      op_rr(SUB_R_RM, dst, src);
    }

    void mul_rr(char dst, char src) {
      op_rr2(IMUL_R_RM, dst, src);
    }

    void div_r(char r) {
      op_rr(IDIV_RM, 7, r);
    }

    void push_r(char r) {
      op_r(PUSH_R, r, true, 1);
    }

    void pop_r(char r) {
      op_r(POP_R, r, true, 1);
    }

    void sub_rm_imm(char r, int32_t v) {
      op_rr(SUB_RM_IMM, 5, r);
      addInt32(v);
    }

    void add_rm_imm(char r, int32_t v) {
      op_rr(ADD_RM_IMM, 0, r);
      addInt32(v);
    }

    void call_r(char r) {
      //add(x86_rex(2, r, 1));
      add(CALL_RM);
      add(x86_modrm(MOD_RR, 2, r));
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

    void add_xmm_xmm(char xmm1, char xmm2) {
      add3(ADDSD);
      add(x86_modrm(MOD_RR, xmm1, xmm2));
    }

    void sub_xmm_xmm(char xmm1, char xmm2) {
      add3(SUBSD);      
      add(x86_modrm(MOD_RR, xmm1, xmm2));
    }

    void mul_xmm_xmm(char xmm1, char xmm2) {
      add3(MULSD);
      add(x86_modrm(MOD_RR, xmm1, xmm2));
    }

    void div_xmm_xmm(char xmm1, char xmm2) {
      add3(DIVSD);
      add(x86_modrm(MOD_RR, xmm1, xmm2));
    }

    void neg_r(char r) {
      add(x86_rex(r, 3, 1));
      add(NEG_RM);
      add(x86_modrm(MOD_RR, 3, r));
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

  private:
    void* _x86code;
    size_t _size;
  };

  typedef X86Code NativeCode;

  // --------------------------------------------------------------------------------

  class NativeFunction : public TranslatedFunction {
  public:
    NativeFunction(AstFunction* node) 
      : TranslatedFunction(node) { }

    NativeCode* code() {
      return &_code;
    }

    void setFirstArg(uint16_t idx) {
      _firstArg = idx;
    }

    void setFirstLocal(uint16_t idx) {
      _firstLocal = idx;
    }

    uint16_t firstLocal() const {
      return _firstLocal;
    }

    uint16_t firstArg() const {
      return _firstArg;
    }

    void disassemble(std::ostream& out) const { }

  private:
    uint16_t _firstArg;
    uint16_t _firstLocal;
    VarType _retType;    

    NativeCode _code;
  };


  class Runtime : public Code {
    typedef std::vector<std::string> Strings;

  public:
    NativeFunction* createFunction(AstFunction* fNode);

    Status* execute(std::vector<mathvm::Var*, std::allocator<Var*> >& args);

    const char* addString(const std::string& s) {
      _strings.push_back(s);
      return _strings.back().c_str();
    }

  private:
    std::deque<NativeFunction*> _functions;
    Strings _strings;
  };
}
