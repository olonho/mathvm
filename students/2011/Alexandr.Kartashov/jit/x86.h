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

static uint16_t x86_cond(uint16_t insn, char cond) {
  if (insn < 0xFF) {
    return insn + cond;
  } else {
    return insn + (cond << 8);
  }
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

#define CC_O       0
#define CC_NO      1
#define CC_B       2
#define CC_C       2
#define CC_NAE     2
#define CC_NB      3
#define CC_NC      3
#define CC_AE      3
#define CC_E       4
#define CC_Z       4
#define CC_NE      5
#define CC_NZ      5
#define CC_BE      6
#define CC_NA      6
#define CC_A       7
#define CC_NBE     7
#define CC_S       8
#define CC_NS      8
#define CC_P       9
#define CC_PE      9
#define CC_NP      0xB
#define CC_L       0xC
#define CC_NGE     0xC
#define CC_GE      0xD
#define CC_NL      0xD
#define CC_LE      0xE
#define CC_NG      0xE
#define CC_G       0xF
#define CC_NLE     0xF

#define SSE_CMP_EQ  0
#define SSE_CMP_LT  1
#define SSE_CMP_LE  2
#define SSE_CMP_NEQ 3
#define SSE_CMP_NLT 5
#define SSE_CMP_NLE 6

#define INT_REGS    16
#define DOUBLE_REGS 8

// ================================================================================
// Instruction encodings

#define ADD_R_RM     0x03
#define MOVLPD       0x130F
#define MOVQ_XMM_RM  0x6E0F
#define MOVQ_RM_XMM  0x7E0F

#define JCC_REL32    0x800F
#define JO_R32       0x800F
#define JNO_R32      0x810F
#define JC_R32       0x820F
#define JB_R32       0x820F
#define JNAE_R32     0x820F
#define JNB_R32      0x830F
#define JAE_R32      0x830F
#define JNC_R32      0x830F
#define JE_R32       0x840F
#define JZ_R32       0x840F
#define JNE_R32      0x850F
#define JNZ_R32      0x850F
#define JBE_R32      0x860F
#define JNA_R32      0x860F
#define JNBE_R32     0x870F
#define JA_R32       0x870F
#define JS_R32       0x880F
#define JNS_R32      0x890F
#define JP_R32       0x8A0F
#define JPE_R32      0x8A0F
#define JNP_R32      0x8B0F
#define JPO_R32      0x8B0F
#define JNGE_R32     0x8C0F
#define JL_R32       0x8C0F
#define JNL_R32      0x8D0F
#define JNG_R32      0x8E0F
#define JLE_R32      0x8E0F
#define JGE_R32      0x8D0F
#define JG_R32       0x8F0F
#define JNLE_R32     0x8F0F

#define SETCC_RM     0x900F
#define SETO_RM      0x900F
#define SETNO_RM     0x910F
#define SETB_RM      0x920F
#define SETC_RM      0x920F
#define SETNAE_RM    0x920F
#define SETAE_RM     0x930F
#define SETNC_RM     0x930F
#define SETNB_RM     0x930F
#define SETE_RM      0x940F
#define SETZ_RM      0x940F
#define SETNE_RM     0x950F
#define SETNZ_RM     0x950F
#define SETBE_RM     0x960F
#define SETNA_RM     0x960F
#define SETA_RM      0x970F
#define SETNBE_RM    0x970F
#define SETS_RM      0x980F
#define SETNS_RM     0x990F
#define SETP_RM      0x9A0F
#define SETPE_RM     0x9A0F
#define SETNP_RM     0x9B0F
#define SETLT_RM     0x9C0F
#define SETNGE_RM    0x9C0F
#define SETGE_RM     0x9D0F
#define SETNL_RM     0x9D0F
#define SETLE_RM     0x9E0F
#define SETNG_RM     0x9E0F
#define SETG_RM      0x9F0F
#define SETNLE_RM    0x9F0F

#define IMUL_R_RM    0xAF0F
#define PSUBD_XMM_XM 0xFA0F 
#define SUB_R_RM     0x2B

// NB: IMM is 32-bit even in the IA32e mode!
// <OddInstructionEncoding>
#define SUB_RM_IMM 0x81  // RO = 5
#define ADD_RM_IMM 0x81  // RO = 0
// </OddInstructionEncoding>

#define XOR_R_RM     0x33
#define CMP_R_RM     0x3B
#define PUSH_R       0x50
#define POP_R        0x58
#define PUSH_IMM     0x68
#define CMP_RM_IMM   0x83  // RO = 7
#define AND_RM_IMM   0x83  // RO = 4
#define TEST_RM_R    0x85
#define MOV_RM_R     0x89
#define MOV_R_RM     0x8B
#define MOVS_B       0xA4
#define MOVS_W       0xA5
#define MOV_R_IMM    0xB8
#define RET          0xC3
#define MOV_RM_IMM   0xC7  // RO = 0
#define CALL_REL32   0xE8
#define JMP_REL32    0xE9
#define ADDSD        0x580FF2
#define MULSD        0x590FF2
#define SUBSD        0x5C0FF2
#define DIVSD        0x5E0FF2
#define CMPSD        0xC20FF2
#define IDIV_RM      0xF7  // RO = 7
#define NEG_RM       0xF7  // RO = 3
#define CALL_RM      0xFF  // RO = 2

#define REPNE        0xF2
