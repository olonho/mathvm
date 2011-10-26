#pragma once

// ================================================================================
// Instruction fields encoding

#define REX_MAGIC  4
#define EX_BITMASK 8

#define MOD_RM_D32 2
#define MOD_RR     3

union X86Bits {
  struct {
    unsigned rm    : 1;
    unsigned x     : 1;
    unsigned ro    : 1;
    unsigned w     : 1;
    unsigned magic : 4;
  } rex;
      
  struct {
    unsigned rm    : 3;
    unsigned ro    : 3;
    unsigned mod   : 2;
  } modrm;

  struct {
    unsigned base  : 3;
    unsigned index : 3;
    unsigned scale : 2;
  } sib;
      
  uint8_t flat;
};
  
static uint8_t x86_modrm(char mod, char ro, char rm) {
  X86Bits b;

  b.modrm.mod = mod;
  b.modrm.ro = ro;
  b.modrm.rm = rm;
  return b.flat;
}

static uint8_t x86_rex(char ro, char rm, char w) {
  X86Bits b;

  b.rex.magic = REX_MAGIC;
  b.rex.w = w;
  b.rex.x = 0;
  b.rex.ro = ro & EX_BITMASK;
  b.rex.rm = rm & EX_BITMASK;
  return b.flat;
}

// ================================================================================
// Register encodings

#define RAX 0
#define RCX 1
#define RDX 2
#define RBX 3
#define RSP 4
#define RBP 5
#define RSI 6
#define RDI 7
#define R8  8
#define R9  9
#define R10 10
#define R11 11
#define R12 12 
#define R13 13
#define R14 14
#define R15 15

#define XMM0 0
#define XMM1 1
#define XMM2 2
#define XMM3 3
#define XMM4 4
#define XMM5 5
#define XMM6 6
#define XMM7 7


#define INT_REGS 16
#define DOUBLE_REGS 8

// ================================================================================
// Instruction encodings

#define ADD_R_RM     0x03
#define MOVLPD       0x130F
#define MOVQ_XMM_RM  0x6E0F
#define MOVQ_RM_XMM  0x7E0F
#define IMUL_R_RM    0xAF0F
#define PSUBD_XMM_XM 0xFA0F 
#define SUB_R_RM     0x2B

// NB: IMM is 32-bit even in the IA32e mode!
// <OddInstructionEncoding>
#define SUB_RM_IMM 0x81  // RO = 5
#define ADD_RM_IMM 0x81  // RO = 0
// </OddInstructionEncoding>

#define MOV_RM_R     0x89
#define MOV_R_RM     0x8B
#define PUSH_R       0x50
#define POP_R        0x58
#define MOV_R_IMM    0xB8
#define RET          0xC3
#define ADDSD        0x580FF2
#define MULSD        0x590FF2
#define SUBSD        0x5C0FF2
#define DIVSD        0x5E0FF2
#define IDIV_RM      0xF7  // RO = 7
#define NEG_RM       0xF7  // RO = 3
#define CALL_RM      0xFF  // RO = 2
