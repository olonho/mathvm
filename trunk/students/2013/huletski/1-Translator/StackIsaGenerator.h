//
//  StackIsaGenerator.h
//  VM_2
//
//  This class incapsulates knowledge about particular ISA
//
//  Created by Hatless Fox on 10/26/13.
//  Copyright (c) 2013 WooHoo. All rights reserved.
//

#ifndef VM_2_StackIsaGenerator_h
#define VM_2_StackIsaGenerator_h

#include <cmath>
#include <map>
#include <string>

#include "mathvm.h"
#include "CommonStructs.h"

using std::map;
using std::string;
using namespace mathvm;

class StackIsaGenerator {
public: // methods

  StackIsaGenerator() { fillInstrMap(); }
  
  void setBC(Bytecode *bc) { m_bc = bc; }
  Bytecode *bc() { return m_bc; }

  inline void print(VarType const &type) {
    m_bc->addInsn(toIns(t2s(type) + "PRINT"));
  }
  
  inline void load(VarInfo * v_i, uint16_t parent_func_scope) {
    genAccess("LOAD", v_i, parent_func_scope);
  }

  inline void store(VarInfo * v_i, uint16_t parent_func_scope) {
    genAccess("STORE", v_i, parent_func_scope);
  }

  inline void convert(VarType from, VarType to) {
    if (from == to) { return; }
    string req_cnv = t2s(from) + "2" + t2s(to);
    if (m_name_to_instr.count(req_cnv)) {
      addInsn(req_cnv);
    }
    
    assert(0); // TODO: throw exception
  }
  
  inline VarType toBroader(VarType top, VarType next) {
    // D > I > S
    if (top == next) { return top; }
    
    
    unsigned top_ind  = top == VT_DOUBLE ? 4 : top == VT_INT ? 2 : 1;
    unsigned next_ind = next == VT_DOUBLE ? 4 : next == VT_INT ? 2 : 1;
    
    if (fabs(top_ind - next_ind) == 3) { // D, S
      return VT_INVALID;
    }
    
    if (top > next_ind) {
      addInsn(BC_SWAP);
      addInsn(t2s(next) + "2" + t2s(top));
      addInsn(BC_SWAP);
      return top;
    } else { // want to convert top
      addInsn(t2s(top) + "2" + t2s(next));
      return next;
    }
  }

  
  inline void tosToInt(VarType top) {
    if (top == VT_INT) { return; }
    addInsn(t2s(top) + "2" + t2s(VT_INT));
  }
    
  inline VarType toInts(VarType top, VarType next) {
    if (next != VT_INT) {
      addInsn(BC_SWAP);
      tosToInt(next);
      addInsn(BC_SWAP);
    }
    
    tosToInt(top);
    return VT_INT;
  }
  
  inline void pushStr(uint16_t s_id) {
    addInsn(BC_SLOAD);
    m_bc->addUInt16(s_id);
  }

  inline void pushDouble(double val) {
    addInsn(BC_DLOAD);
    m_bc->addDouble(val);
  }
  
  inline void pushInt(uint64_t val) {
    addInsn(BC_ILOAD);
    m_bc->addInt64(val);
  }

  inline void add(VarType type) { addInsn(t2s(type) + "ADD"); }
  inline void sub(VarType type) { addInsn(t2s(type) + "SUB"); }
  inline void mul(VarType type) { addInsn(t2s(type) + "MUL"); }
  inline void div(VarType type) { addInsn(t2s(type) + "DIV"); }
  inline void mod(VarType type) { addInsn(t2s(type) + "MOD"); }
  inline void neg(VarType type) { addInsn(t2s(type) + "NEG"); }
  inline void cmp(VarType type) { addInsn(t2s(type) + "CMP"); }
  
  inline void aand() { addInsn(BC_IAAND); }
  inline void aor() { addInsn(BC_IAOR); }

  inline void axor() { addInsn(BC_IAXOR); }

  
  inline void lor() {
    addInsn(BC_IADD);
    genBrachedIntSet(VT_INT, 0, 0, 1);
  }
  
  inline void land() {
    addInsn(BC_IMUL);
    genBrachedIntSet(VT_INT, 1, 1, 0);
  }

  inline void leq(VarType tos_type) {
    cmp(tos_type);
    genBrachedIntSet(VT_INT, 0, 1, 0);
  }
  
  inline void lneq(VarType tos_type) {
    cmp(tos_type);
    genBrachedIntSet(VT_INT, 0, 0, 1);
  }
  
  inline void lge(VarType tos_type) {
    cmp(tos_type);
    genBrachedIntSet(VT_INT, -1, 0, 1);
  }
  
  inline void lgt(VarType tos_type) {
    cmp(tos_type);
    genBrachedIntSet(VT_INT, 1, 1, 0);
  }

  inline void lle(VarType tos_type) {
    cmp(tos_type);
    genBrachedIntSet(VT_INT, 1, 0, 1);
  }
  inline void llt(VarType tos_type) {
    cmp(tos_type);
    genBrachedIntSet(VT_INT, -1, 1, 0);
  }

  inline void nt(VarType tos_type) { genBrachedIntSet(tos_type, 0, 1, 0); }
  
  void tosTrueCheck(Label *check_fail_code) {
    addInsn(BC_ILOAD0);
    m_bc->addBranch(BC_IFICMPE, *check_fail_code);
  }
  
  
  inline void addInsn(Instruction insn) { m_bc->addInsn(insn); }
  inline void addUInt16(uint16_t offset) { m_bc->addUInt16(offset); }
  
  inline void ifStmnt(VarType cond_tos, Bytecode *cond,
                      Bytecode *then_blk, Bytecode *else_blk) {
    
    Label else_lbl(m_bc);
    
    addBytecodeToCurrent(cond);
    
    tosToInt(cond_tos);
    tosTrueCheck(&else_lbl);

    addBytecodeToCurrent(then_blk);
    
    if (else_blk) {
      Label eos_lbl(m_bc); // end of statement label
      m_bc->addBranch(BC_JA, eos_lbl);
      m_bc->bind(else_lbl);
      addBytecodeToCurrent(else_blk);
      m_bc->bind(eos_lbl);
    } else {
      m_bc->bind(else_lbl);
    }
  }
  
  
  inline void whileStmnt(VarType cond_tos, Bytecode *cond, Bytecode *body) {
    Label loop_end(m_bc);
    Label loop_start = m_bc->currentLabel();
    
    addBytecodeToCurrent(cond);
    
    tosToInt(cond_tos);
    tosTrueCheck(&loop_end);
    
    addBytecodeToCurrent(body);
    
    m_bc->addInsn(BC_JA);
    m_bc->addInt16(loop_start.offsetOf(m_bc->current()));
    m_bc->bind(loop_end);
  }
  
  inline void forStmnt(VarInfo *ind_var_info, uint16_t fn_sc_id,
                       Bytecode *init_val, Bytecode *last_val,
                       Bytecode *body_blk) {

    addBytecodeToCurrent(init_val);
    store(ind_var_info, fn_sc_id);
    
    Label loop_start = m_bc->currentLabel();
    Label loop_end(m_bc);
    load(ind_var_info, fn_sc_id);
    addBytecodeToCurrent(last_val);
    
    pushInt(1);
    add(VT_INT);
    
    cmp(ind_var_info->type);
    tosTrueCheck(&loop_end);
    addBytecodeToCurrent(body_blk);
    
    load(ind_var_info, fn_sc_id);
    pushInt(1);
    add(VT_INT);
    store(ind_var_info, fn_sc_id);
    
    addInsn(BC_JA);
    m_bc->addInt16(loop_start.offsetOf(m_bc->current()));
    m_bc->bind(loop_end);
  }
  
private: // methods
  
  void addBytecodeToCurrent(Bytecode *bc) {
    for (uint32_t i = 0; i < bc->length(); i += 1) {
      m_bc->add(bc->getByte(i));
    }
  }
  
  string t2s(VarType const & type) {
    switch (type) {
      case VT_DOUBLE: return "D";
      case VT_INT: return "I";
      case VT_STRING: return "S";
        
      default:
        assert(0); //TODO: throw excptn
    }
  }

  inline void addInsn(string const &name) { m_bc->addInsn(toIns(name)); }
  inline Instruction toIns(string const &str) { return m_name_to_instr[str]; }
  
  void fillInstrMap() {
  #define GEN_ADD_ENTRY(b, d, l) m_name_to_instr[ #b ] = BC_##b;
    FOR_BYTECODES(GEN_ADD_ENTRY)
  #undef ENUM_ELEM
  }
  
  void genAccess(string const & type, VarInfo * v_i, uint16_t parent_func_scope) {
    if (v_i->scope_id != parent_func_scope) {
      addInsn(type + "CTX" + t2s(v_i->type) + "VAR");
      m_bc->addUInt16(v_i->scope_id);
    } else {
      addInsn(type + t2s(v_i->type) + "VAR");
    }
    m_bc->addUInt16(v_i->local_ind);
  }
  
  void genBrachedIntSet(VarType tos_type, uint64_t expected, uint64_t ok_val, uint64_t not_ok_val) {
    Label else_lbl(m_bc);
    
    tosToInt(tos_type);
    addInsn(BC_ILOAD);
    m_bc->addInt64(expected);
    addInsn(BC_ICMP);
    tosTrueCheck(&else_lbl);
    addInsn(BC_ILOAD);
    m_bc->addInt64(not_ok_val);
    
    Label eos_lbl(m_bc); // end of statement label
    m_bc->addBranch(BC_JA, eos_lbl);
    m_bc->bind(else_lbl);
    addInsn(BC_ILOAD);
    m_bc->addInt64(ok_val);
    m_bc->bind(eos_lbl);
  }
  
  
private: //fields
  map<string, Instruction> m_name_to_instr;
  Bytecode *m_bc;
};

#endif
