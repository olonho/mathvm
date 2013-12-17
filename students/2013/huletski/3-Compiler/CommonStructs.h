//
//  CommonStructs.h
//  VM_3
//
//  Created by Hatless Fox on 12/13/13.
//  Copyright (c) 2013 WooHoo. All rights reserved.
//

#ifndef VM_3_CommonStructs_h
#define VM_3_CommonStructs_h

#include "ast.h"
#include "mathvm.h"

#ifdef XCODE_BUILD
#include "AsmJit.h"
#else
#include <AsmJit/AsmJit.h>
#endif

using namespace AsmJit;
using namespace mathvm;

struct FirstPassVarInfo {
  uint16_t func_id;
  uint64_t scope_id;
  bool global;
  uint64_t globals_ind;
  FirstPassVarInfo(uint16_t fn_id, uint64_t sc_id, bool glbl):
    func_id(fn_id),scope_id(sc_id), global(glbl) {}
};
typedef FirstPassVarInfo FPVI;

struct AstAsmVarInfo {
  BaseVar *aj_var; // if type is DBL -- XMMVar, overwise GPVar
  VarType type;
  bool is_transient; // true if holds temporal result
  bool is_global;
  uint64_t global_ind;
  uint16_t fn_id;
  
  AstAsmVarInfo(bool transient, Compiler & c, VarType t,
                bool is_glbl = false, uint64_t glbl_i = 0):
    type(t), is_global(is_glbl), global_ind(glbl_i), m_c(c){
    
    is_transient = transient;
    if (t == VT_DOUBLE) { aj_var = new XMMVar(c.newXMM(VARIABLE_TYPE_XMM_1D));}
    else { aj_var = new GPVar(c.newGP(VARIABLE_TYPE_INT64)); }
  }
  
  ~AstAsmVarInfo () {
    m_c.unuse(*aj_var);
    delete aj_var;
  }
  
  AstAsmVarInfo *clone() {
    AstAsmVarInfo * cl = new AstAsmVarInfo(false, m_c, type, false, 0);
    delete cl->aj_var;
    if (type == VT_DOUBLE) { cl->aj_var = new XMMVar(*(XMMVar *)aj_var); }
    else { cl->aj_var = new GPVar(*(GPVar *)aj_var); }
    return cl;
  }
  
private:
  Compiler &m_c;
};

typedef AstAsmVarInfo AAVI;

union EvalResulValue{
  AAVI *var;
  int64_t i;
  double d;
  char *str;
};

typedef EvalResulValue ERV;

//holds result of last operation
struct EvalResult {
  EvalResulValue data;
  VarType type;
  
  VarType effectiveType() { return type == VT_INVALID ? data.var->type : type; }
  
  EvalResult(VarType t, EvalResulValue d): data(d), type(t) { }
  bool isConstant() { return type != VT_INVALID; }
};

#endif
