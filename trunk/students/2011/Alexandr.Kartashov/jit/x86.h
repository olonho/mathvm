#pragma once

// ================================================================================
// Instruction fields encoding

#define REX_MAGIC 4
#define EX_BITMASK 8

#define MOD_RR 3

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
      
  uint8_t flat;
};
  
static uint8_t x86_modrm(char mod, char ro, char rm) {
  X86Bits b;

  b.modrm.mod = MOD_RR;
  b.modrm.ro = ro;
  b.modrm.rm = rm;
  return b.flat;
}

static uint8_t x86_rex(char ro, char rm, char w) {
  X86Bits b;

  b.rex.magic = REX_MAGIC;
  b.rex.w = w;
  b.rex.ro = ro & EX_BITMASK;
  b.rex.rm = rm & EX_BITMASK;
  return b.flat;
}

// ================================================================================
// Register encodings

#define RAX 0
#define RCX 1
#define RBP 5

// ================================================================================
// Instruction encodings

#define ADD_R_RM  0x03
#define IMUL_R_RM 0xAF0F
#define SUB_R_RM  0x2B
#define MOV_R_RM  0x8B
#define PUSH_R    0x50
#define POP_R     0x58
#define MOV_R_IMM 0xB8
#define RET       0xC3
