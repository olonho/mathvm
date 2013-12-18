//
//  RegistryISAGenerator.h
//  VM_3
//
//  Created by Hatless Fox on 12/11/13.
//  Copyright (c) 2013 WooHoo. All rights reserved.
//

#ifndef VM_3_RegistryISAGenerator_h
#define VM_3_RegistryISAGenerator_h

#include <cmath>
#include <map>
#include <string>
#include <cfloat>
#include <cstdlib>
#include <dlfcn.h>
#include <sstream>
#include <stack>

#include "CommonStructs.h"
#include "mathvm.h"

#ifdef XCODE_BUILD
#include "AsmJit.h"
#else
#include <AsmJit/AsmJit.h>
#endif

using std::map;
using std::string;
using namespace mathvm;
using namespace AsmJit;

#include "RegistryISAStaticHelpers.h"

#define AS_XMM(aavi) *((XMMVar *)(aavi)->aj_var)
#define AS_GP(aavi) *((GPVar *)(aavi)->aj_var)

class RegistryISAGenerator {
public: // methods
  
  RegistryISAGenerator():m_error_status(NULL) {
    //m_compiler.setLogger(new FileLogger(stdout));
    
    m_prnt_code = dlsym(RTLD_DEFAULT, "printf");
    m_gvt = NULL;
    assert(m_prnt_code && "Unable to obtain printf code");
  }
  
  AAVI * asmVarInfo(VarType type, FPVI *v_i) {
    AAVI *aavi = new AAVI(false, m_compiler, type,
                          v_i->global, v_i->globals_ind);
    aavi->fn_id = v_i->func_id;
    return aavi;
  }
  
  void * generateAsm() {
    void *code = m_compiler.make();
    assert(code && "ISA Unable to generate code");
    return code;
  }

  inline void setFunction(EFunction *func) { m_func = func;}
  inline EFunction * function() { return m_func; }
  
  inline void setFuncsCnt(uint64_t funcs_cnt) {
    if (!funcs_cnt) { return; }
    m_gvt = new uint64_t[funcs_cnt]();
  }

  
  //WORKAROUND: for div late init issue ->
  //            set divisor ASAP (looks like AsmJit issue)
  // Examle of Bad generated asm:
  /*
   __asm__ ( "mov $42,%rax;"
   "mov $42,%rdx;"
   "mov $0, %rcx;"
   "mov %rdx, %rcx;"
   "idiv %rcx;"
   );
   */
  EFunction * newFunction(Signature sign) {
    EFunction * f = m_compiler.newFunction(CALL_CONV_DEFAULT, convToFB(sign));
    m_ret_lbl = new AsmJit::Label(m_compiler.newLabel());
    
    switch (sign[0].first) {
      case VT_VOID: m_ret_var = NULL; break;
      case VT_STRING:
      case VT_INT: m_ret_var = new GPVar(m_compiler.newGP()); break;
      case VT_DOUBLE: m_ret_var = new XMMVar(m_compiler.newXMM()); break;
      default: break;
    }
    
    m_divisor = new AAVI(true, m_compiler, VT_INT);
    m_compiler.setPriority(*(m_divisor->aj_var), 100);
    m_gp_tmp = new GPVar(m_compiler.newGP(VARIABLE_TYPE_INT64));
    return f;
  }
  
  void finalizeFuncProcessing(VarType ret_type) {
    m_compiler.bind(*m_ret_lbl);
    
    switch (ret_type) {
      case VT_VOID: m_compiler.ret(); break;
      case VT_STRING:case VT_INT: m_compiler.ret(*(GPVar *)m_ret_var); break;
      case VT_DOUBLE: m_compiler.ret(*(XMMVar *)m_ret_var); break;
      default: break;
    }
    
    m_compiler.unuse(*m_gp_tmp);
    m_compiler.endFunction();
    delete m_gp_tmp;
    
    m_gp_tmp  = NULL;
    m_ret_lbl = NULL;
    m_ret_var = NULL;
    m_divisor = NULL;
  }
  
  void performTopGVTPush(uint16_t fn_id, uint64_t fn_globals) {
    if (fn_globals) { pushGVT((uint64_t)m_gvt, fn_id, fn_globals); }
  }
  
  inline Status* errorStatus() { return m_error_status; }
 
//------------------------------------------------------------------------------
// Build-in Commands
  
  inline void print(std::vector<EvalResult> args, Code *code) {
    std::string fmt_str;
    std::vector<AAVI *> vars;
    FunctionBuilderX fn_bldr;
    fn_bldr.setReturnValue<void>();
    fn_bldr.addArgument<char *>(); // for forat string
    
    for (std::vector<EvalResult>::iterator it = args.begin(); it != args.end(); ++it) {
      EvalResult er = *it;
      if (er.type == VT_INVALID) { //invalid is for variables
        fmt_str += prnt_fmt_spec(er.data.var->type);
        vars.push_back((AAVI *)er.data.var);
        switch (er.data.var->type) {
          case VT_DOUBLE: fn_bldr.addArgument<double>(); break;
          case VT_INT:    fn_bldr.addArgument<int64_t>();    break;
          case VT_STRING: fn_bldr.addArgument<char *>(); break;
          default: assert(0 && "Unexpected type");
        }
      } else {
        #define ADD_TO_FMT(dt) std::ostringstream ss; ss << dt; fmt_str += ss.str();
        switch (er.type) {
          case VT_DOUBLE: { ADD_TO_FMT(er.data.d); break; }
          case VT_INT:    { ADD_TO_FMT(er.data.i); break; }
          case VT_STRING: { ADD_TO_FMT(er.data.str); break; }
          default: assert(0 && "Unexpected type");
        }
        #undef ADD_TO_FMT
      }
    }
    
    // Looks like memleak, but we have to be sure that fmt_str has always
    // same location and lifetime of application, so just store it in heap
    char *fmt_cpy = new char[fmt_str.size() + 1]();
    memcpy(fmt_cpy, fmt_str.c_str(), fmt_str.size() + 1);

    GPVar spec = m_compiler.newGP();
    m_compiler.mov(spec, imm((sysint_t)fmt_cpy));
    
    //WORKAROUND: load addr to register before call to print doubles
    GPVar native_addr(m_compiler.newGP());
    m_compiler.mov(native_addr, imm((sysint_t)m_prnt_code));
    ECall *ctx = m_compiler.call(native_addr);
    m_compiler.unuse(native_addr);
    
    ctx->setPrototype(CALL_CONV_DEFAULT, fn_bldr);
    ctx->setArgument(0, spec);

    for (uint32_t arg_i = 0; arg_i < vars.size(); ++arg_i) {
      ctx->setArgument(arg_i + 1, *(vars[arg_i]->aj_var));
    }
  }

//------------------------------------------------------------------------------
// Conversions
  
  void toInt(EvalResult *value) {
    if (value->type == VT_INVALID) { // value is stored in var
      AAVI *aavi = (AAVI *)value->data.var;
      switch (aavi->type) {
        case VT_STRING: case VT_INT: break;          // INT, STR -> do nothing
        case VT_DOUBLE: {                            // DBL -> store to GPVar
          XMMVar &xmm = AS_XMM(aavi);
          
          XMMVar tmp(m_compiler.newXMM(VARIABLE_TYPE_XMM_1D));
          GPVar *int_holder = new GPVar(m_compiler.newGP(VARIABLE_TYPE_INT64));
          m_compiler.cvtpd2dq(tmp, xmm);
          m_compiler.movq(*int_holder, tmp);
          m_compiler.unuse(tmp);
          delete aavi->aj_var;
          aavi->aj_var = int_holder;
          break;
        }
        default: assert(0 && "WTF type"); break;
      }
      aavi->type = VT_INT;
    } else { //constants
      switch (value->type) {
        case VT_STRING: case VT_INT: break;
        case VT_DOUBLE: value->data.i = value->data.d; break;
        default: assert(0 && "WTF type"); break;
      }
      value->type = VT_INT;
    }
  }
  
  inline void toInts(EvalResult *a, EvalResult *b) { toInt(a); toInt(b); }
  
  void toDbl(EvalResult *value) {
    if (value->type == VT_INVALID) { // value is stored in var
      AAVI *aavi = (AAVI *)value->data.var;
      switch (aavi->type) {
        case VT_DOUBLE: break;
        case VT_STRING: logError("Cant Cast Str to Dbl"); break;
        case VT_INT: {
          GPVar &int_holder = AS_GP(aavi);
          XMMVar *dbl_holder = new XMMVar(m_compiler.newXMM(VARIABLE_TYPE_XMM_1D));
          
          m_compiler.movq(*dbl_holder, int_holder);
          m_compiler.cvtdq2pd(*dbl_holder, *dbl_holder);
          delete aavi->aj_var;
          aavi->aj_var = dbl_holder;
          break;
        }
        default: logError("Unexpected value type"); break;
      }
      aavi->type = VT_DOUBLE;
    } else { //constants
      switch (value->type) {
        case VT_DOUBLE: break;
        case VT_STRING: logError("Cant Cast Str to Dbl"); break;
        case VT_INT: value->data.d = value->data.i; break;
        default: logError("Unexpected value type"); break;
      }
      value->type = VT_DOUBLE;
    }
  }
  
  inline VarType toBroader(EvalResult *l, EvalResult *r) {
    // D > I > S
    VarType l_type = l->effectiveType();
    VarType r_type = r->effectiveType();
    if (l_type == r_type) { return l_type; }
    
    long l_ind = l_type == VT_DOUBLE ? 4 : l_type == VT_INT ? 2 : 1;
    long r_ind = r_type == VT_DOUBLE ? 4 : r_type == VT_INT ? 2 : 1;
    
    if (abs(l_ind - r_ind) == 3) { // D, S
      logError("Unable to convert type str to double or vice versa");
      return VT_INVALID;
    }
    
    VarType broader_type = (l_ind > r_ind) ? l_type : r_type;
    switch (broader_type) {
      case VT_DOUBLE: { toDbl(l); toDbl(r); break; }
      case VT_INT: { toInts(l, r); break; }
      default: logError("Unexpected broader type");
    }
    return broader_type;
  }
  
  void toBool(AAVI *aavi) {
    m_compiler.cmp(AS_GP(aavi), imm(0));
    (AS_GP(aavi))._var.size = 1;
    m_compiler.setnz(AS_GP(aavi));
    (AS_GP(aavi))._var.size = 8;
    m_compiler.and_(AS_GP(aavi), imm(1));
  }

  #define LOAD_CONST_TO_XMM(xmm_var, const)                                    \
    m_compiler.mov(*m_gp_tmp, imm(d2i((double)const)));                              \
    m_compiler.movq(xmm_var, *m_gp_tmp);

  void ensureInVar(EvalResult *er) {
    if (er->type == VT_INVALID) { return; }
    if (er->type == VT_DOUBLE) {
      AAVI *aavi = new AAVI(false, m_compiler, VT_DOUBLE);
      LOAD_CONST_TO_XMM(AS_XMM(aavi), (er->data.d))
      er->type = VT_INVALID;
      er->data.var = aavi;
    } else {
      AAVI *aavi = new AAVI(false, m_compiler, VT_INT);
      m_compiler.mov(AS_GP(aavi), imm(er->data.i));
      er->type = VT_INVALID;
      er->data.var = aavi;
    }
  }
  
//------------------------------------------------------------------------------
// Int Operations
  
  #define HAS_TRANSIENT_VAR(er)                                    \
    er->type == VT_INVALID && er->data.var->is_transient
  
  #define TRY_INT_CONST_COMPUTE(a, op, b)                          \
    if ((a)->type != VT_INVALID && (b)->type != VT_INVALID) {      \
      EvalResulValue erv; erv.i = (a)->data.i op (b)->data.i;      \
      return EvalResult(VT_INT, erv);                              \
    }

  #define INT_PERFORM_OP(dest, src_aavi, op)                       \
    ((src_aavi)->type == VT_INVALID) ?                             \
      m_compiler.op(dest, AS_GP((src_aavi)->data.var)) :           \
      m_compiler.op(dest, imm((src_aavi)->data.i));

  #define INT_EMIT_COMMUT_OP_CODE(a, b, res, op, def)              \
    if (HAS_TRANSIENT_VAR(a)) {                                    \
      res = a->data.var;                                           \
      INT_PERFORM_OP(AS_GP(res), b, op)                            \
      if (HAS_TRANSIENT_VAR(b)) { delete b->data.var; }            \
    } else if (HAS_TRANSIENT_VAR(b)) {                             \
      res = b->data.var;                                           \
      INT_PERFORM_OP(AS_GP(res), a, op)                            \
    } else {                                                       \
      res = new AAVI(true, m_compiler, VT_INT);                    \
      m_compiler.mov(AS_GP(res), imm(def));                        \
      INT_PERFORM_OP(AS_GP(res), a, op)                            \
      INT_PERFORM_OP(AS_GP(res), b, op)                            \
    }

  #define GENERATE_INT_COMMUTATIVE_CMD(name, op, asm_op, def)      \
    inline EvalResult name(EvalResult *l, EvalResult *r) {         \
      TRY_INT_CONST_COMPUTE(l, op, r)                              \
      AAVI *aavi = NULL;                                           \
      INT_EMIT_COMMUT_OP_CODE(l, r, aavi, asm_op, def)             \
      ERV erv; erv.var = aavi;                                     \
      return EvalResult(VT_INVALID, erv);                          \
    }
  
  inline EvalResult lor(EvalResult *l, EvalResult *r) {
    TRY_INT_CONST_COMPUTE(l, ||, r)
    AAVI *aavi = NULL;
    INT_EMIT_COMMUT_OP_CODE(l, r, aavi, or_, 0)
    toBool(aavi);
    ERV erv; erv.var = aavi;
    return EvalResult(VT_INVALID, erv);
  }

  inline EvalResult land(EvalResult *l, EvalResult *r) {
    TRY_INT_CONST_COMPUTE(l, &&, r)
    AAVI *aavi = NULL;
    INT_EMIT_COMMUT_OP_CODE(l, r, aavi, and_, (uint64_t)-1)
    toBool(aavi);
    ERV erv; erv.var = aavi;
    return EvalResult(VT_INVALID, erv);
  }
  
  // Bitwise
  GENERATE_INT_COMMUTATIVE_CMD(aor, |, or_, 0)
  GENERATE_INT_COMMUTATIVE_CMD(aand, &, and_, (uint64_t)-1)
  GENERATE_INT_COMMUTATIVE_CMD(axor, ^, xor_, 0)
  
  inline EvalResult mod(EvalResult *l, EvalResult *r) {
    return int_divide(l, r, true);
  }
 
//------------------------------------------------------------------------------
// Common Operations
  
  #define TRY_CONST_COMPUTE(a, op, b, res_type)                                \
    if ((a)->type != VT_INVALID && (b)->type != VT_INVALID) {                  \
      EvalResulValue erv;                                                      \
      if (res_type == VT_INT) { erv.i = (a)->data.i op (b)->data.i; }          \
      else { erv.d = (a)->data.d op (b)->data.d; }                             \
      return EvalResult(res_type, erv);                                        \
    }
  
  #define PERFORM_GENERIC_OP(dest, src_aavi, int_op, dbl_op, tp)               \
    if (tp == VT_DOUBLE) {                                                     \
      if (src_aavi->type == VT_INVALID) {                                      \
        m_compiler.dbl_op(AS_XMM(dest), AS_XMM(src_aavi->data.var));           \
      } else {                                                                 \
        XMMVar dbl_tmp(m_compiler.newXMM(VARIABLE_TYPE_XMM_1D));               \
        LOAD_CONST_TO_XMM(dbl_tmp, src_aavi->data.d)                           \
        m_compiler.dbl_op(AS_XMM(dest), dbl_tmp);                              \
        m_compiler.unuse(dbl_tmp);                                             \
      }                                                                        \
    } else {                                                                   \
      if (src_aavi->type == VT_INVALID) {                                      \
        m_compiler.int_op(AS_GP(dest), AS_GP(src_aavi->data.var));             \
      } else {                                                                 \
        m_compiler.mov(*m_gp_tmp, imm(src_aavi->data.i));                      \
        m_compiler.int_op(AS_GP(dest), *m_gp_tmp);                             \
      }                                                                        \
    }

  #define GENERATE_COMMUTATIVE_CMD(name, op, asm_op_i, asm_op_d, def)          \
    inline EvalResult name(EvalResult *l, EvalResult *r) {                     \
      VarType type = toBroader(l, r);                                          \
      if (type == VT_INVALID) {                                                \
        ERV erv; erv.var = 0;                                                  \
        return EvalResult(VT_VOID, erv);                                       \
      }                                                                        \
      TRY_CONST_COMPUTE(l, op, r, type)                                        \
                                                                               \
      AAVI *aavi = NULL;                                                       \
      if (HAS_TRANSIENT_VAR(l)) {                                              \
        aavi = l->data.var;                                                    \
        PERFORM_GENERIC_OP(aavi, r, asm_op_i, asm_op_d, type)                  \
        if (HAS_TRANSIENT_VAR(r)) { delete r->data.var; }                      \
      } else if (HAS_TRANSIENT_VAR(r)) {                                       \
        aavi = r->data.var;                                                    \
        PERFORM_GENERIC_OP(aavi, l, asm_op_i, asm_op_d, type)                  \
      } else {                                                                 \
        aavi = new AAVI(true, m_compiler, type);                               \
        if (type == VT_DOUBLE) {  LOAD_CONST_TO_XMM(AS_XMM(aavi), def) }       \
        else { m_compiler.mov(AS_GP(aavi), imm(def)); }                        \
        PERFORM_GENERIC_OP(aavi, l, asm_op_i, asm_op_d, type)                  \
        PERFORM_GENERIC_OP(aavi, r, asm_op_i, asm_op_d, type)                  \
      }                                                                        \
      ERV erv; erv.var = aavi;                                                 \
      return EvalResult(VT_INVALID, erv);                                      \
    }

  GENERATE_COMMUTATIVE_CMD(add, +, add, addpd, 0)
  GENERATE_COMMUTATIVE_CMD(mul, *, imul, mulpd, 1)
  
  inline EvalResult sub(EvalResult *l, EvalResult *r) {
    VarType type = toBroader(l, r);
    if (type == VT_INVALID) {
      ERV erv; erv.var = 0;
      return EvalResult(VT_VOID, erv);
    }

    TRY_CONST_COMPUTE(l, -, r, type)
    
    AAVI *aavi = NULL;
    if (HAS_TRANSIENT_VAR(l)) {
      aavi = l->data.var;
      PERFORM_GENERIC_OP(aavi, r, sub, subpd, type)
    } else {
      aavi = new AAVI(true, m_compiler, type);
      copyContentToVar(l->data.var, aavi);
      PERFORM_GENERIC_OP(aavi, r, sub, subpd, type)
    }
    if (HAS_TRANSIENT_VAR(r)) { delete r->data.var; }
    ERV erv; erv.var = aavi;
    return EvalResult(VT_INVALID, erv);
  }
  
  inline EvalResult div(EvalResult *l, EvalResult *r) {
    VarType type = toBroader(l, r);
    if (type == VT_INVALID) {
      ERV erv; erv.var = 0;
      return EvalResult(VT_VOID, erv);
    }
    TRY_CONST_COMPUTE(l, /, r, type)
    
    if (type != VT_DOUBLE) { return int_divide(l, r, false); }
    
    AAVI *aavi = new AAVI(true, m_compiler, VT_DOUBLE);
    if (l->type == VT_INVALID) {
      m_compiler.movq(AS_XMM(aavi), AS_XMM(l->data.var));
    } else {
      LOAD_CONST_TO_XMM(AS_XMM(aavi), l->data.d)
    }
    PERFORM_GENERIC_OP(aavi, r, sub /* unrelevant */, divpd, VT_DOUBLE)
    if (HAS_TRANSIENT_VAR(r)) { delete r->data.var; }

    ERV erv; erv.var = aavi;
    return EvalResult(VT_INVALID, erv);
  }

  #define GENERATE_COMPARATOR(name, op, mode)                                  \
    inline EvalResult name(EvalResult *l, EvalResult *r) {                     \
      VarType type = toBroader(l, r);                                          \
      if (type == VT_INVALID) {                                                \
        ERV erv; erv.var = 0;                                                  \
        return EvalResult(VT_VOID, erv);                                       \
      }                                                                        \
      TRY_CONST_COMPUTE(l, op, r, type)                                        \
      ensureInVar(l); ensureInVar(r);                                          \
                                                                               \
      AAVI *aavi = new AAVI(true, m_compiler, VT_INT);                         \
      m_compiler.mov(AS_GP(aavi), imm(mode));                                  \
      sysint_t addr = type == VT_DOUBLE ? (sysint_t) &double_cmp :             \
                                          (sysint_t) &int_cmp;                 \
      ECall *ctx = m_compiler.call(imm(addr));                                 \
                                                                               \
      if (type == VT_DOUBLE) {                                                 \
        ctx->setPrototype(CALL_CONV_DEFAULT,                                   \
            FunctionBuilder3<uint64_t, double, double, uint64_t>());           \
        ctx->setArgument(0, AS_XMM(l->data.var));                              \
        ctx->setArgument(1, AS_XMM(r->data.var));                              \
      } else {                                                                 \
        ctx->setPrototype(CALL_CONV_DEFAULT,                                   \
            FunctionBuilder3<uint64_t, uint64_t, uint64_t, uint64_t>());       \
        ctx->setArgument(0, AS_GP(l->data.var));                               \
        ctx->setArgument(1, AS_GP(r->data.var));                               \
      }                                                                        \
                                                                               \
      ctx->setArgument(2, AS_GP(aavi));                                        \
      ctx->setReturn(AS_GP(aavi));                                             \
                                                                               \
      ERV erv; erv.var = aavi;                                                 \
      return EvalResult(VT_INVALID, erv);                                      \
    }

  GENERATE_COMPARATOR(ll, <, 0)
  GENERATE_COMPARATOR(lle, <=, 1)
 // GENERATE_COMPARATOR(leq, ==, 2)
  GENERATE_COMPARATOR(lne, !=, (3))
  GENERATE_COMPARATOR(lge, >=, 4)
  GENERATE_COMPARATOR(lg, >, 5)
  
  inline EvalResult leq(EvalResult *l, EvalResult *r) {
    VarType type = toBroader(l, r);
    if (type == VT_INVALID) {
      ERV erv; erv.var = 0;
      return EvalResult(VT_VOID, erv);
    }
    TRY_CONST_COMPUTE(l, ==, r, type)
    ensureInVar(l); ensureInVar(r);

    AAVI *aavi = new AAVI(true, m_compiler, VT_INT);
    if (type == VT_DOUBLE) {
      m_compiler.mov(AS_GP(aavi), imm(2));

      ECall *ctx = m_compiler.call(imm((sysint_t) &double_cmp));
      ctx->setPrototype(CALL_CONV_DEFAULT,
                        FunctionBuilder3<uint64_t, double, double, uint64_t>());
      ctx->setArgument(0, AS_XMM(l->data.var));
      ctx->setArgument(1, AS_XMM(r->data.var));
      ctx->setArgument(2, AS_GP(aavi));
      ctx->setReturn(AS_GP(aavi));
    } else {
      m_compiler.cmp(AS_GP(l->data.var), AS_GP(r->data.var));
      (AS_GP(aavi))._var.size = 1;
      m_compiler.setz(AS_GP(aavi));
      (AS_GP(aavi))._var.size = 8;
      m_compiler.and_(AS_GP(aavi), imm(1));
    }
    ERV erv; erv.var = aavi;
    return EvalResult(VT_INVALID, erv);
  }
  
  void neg(EvalResult *er) {
    ensureInVar(er);
    if (!(HAS_TRANSIENT_VAR(er))) {
      AAVI *aavi = new AAVI(true, m_compiler, er->data.var->type);
      copyContentToVar(er->data.var, aavi);
      er->data.var = aavi;
    }
    
    if (er->effectiveType() == VT_DOUBLE) {
      AAVI *aavi = new AAVI(true, m_compiler, VT_DOUBLE);
      LOAD_CONST_TO_XMM(AS_XMM(aavi), (-1))
      m_compiler.mulpd(AS_XMM(er->data.var), AS_XMM(aavi));
    } else {
      m_compiler.neg(AS_GP(er->data.var));
    }
  }
  
  void nt(EvalResult *er) {
    toInt(er);
    ensureInVar(er);
    m_compiler.not_(AS_GP(er->data.var));
  }
  
//------------------------------------------------------------------------------
// Emittable Tracking

  void mark() { m_marked_emitable.push(m_compiler.getCurrentEmittable()); }
  
  std::vector<Emittable *> restore() {
    assert(!m_marked_emitable.empty());
    std::vector<Emittable *> stored;
    
    Emittable *marked_e = m_marked_emitable.top();
    Emittable *ptr = marked_e->getNext();
    while (ptr) {
      stored.push_back(ptr);
      m_compiler.removeEmittable(ptr);
      ptr = marked_e->getNext();
    }
    m_marked_emitable.pop();
    return stored;
  }

//------------------------------------------------------------------------------
// Function calls
  
  AAVI * initArg(uint32_t i, VarType arg_type, FPVI *v_i) {
    AAVI *aavi = new AAVI(false, m_compiler, arg_type,
                          v_i->global, v_i->globals_ind);
    delete aavi->aj_var;
    
    aavi->aj_var = (arg_type == VT_DOUBLE) ?
      (BaseVar *) new XMMVar(m_compiler.argXMM(i)) :
      (BaseVar *) new GPVar(m_compiler.argGP(i));
    aavi->fn_id = v_i->func_id;
    updateGVT(aavi);
    return aavi;
  }
  
  inline ECall * call(Signature sign, uint16_t fn_id, uint64_t fn_globals) {
    //setup GVT
    if (fn_globals) {
      GPVar gvt_param(m_compiler.newGP());
      m_compiler.mov(gvt_param, imm((uint64_t)m_gvt));
      GPVar fn_id_param(m_compiler.newGP(VARIABLE_TYPE_INT64));
      m_compiler.mov(fn_id_param, imm((uint64_t)fn_id));
      GPVar var_cnt_param(m_compiler.newGP(VARIABLE_TYPE_INT64));
      m_compiler.mov(var_cnt_param, imm(fn_globals));
    
      sysint_t addr = (sysint_t)&pushGVT;
      ECall *ctx = m_compiler.call(imm(addr));
    
      ctx->setPrototype(CALL_CONV_DEFAULT,
                        FunctionBuilder3<void, uint64_t, uint64_t, uint64_t>());
      ctx->setArgument(0, gvt_param);
      ctx->setArgument(1, fn_id_param);
      ctx->setArgument(2, var_cnt_param);
    }
  
    //create call handler
    ECall *ctx_desc = m_compiler.call(0);
    ctx_desc->setPrototype(CALL_CONV_DEFAULT, convToFB(sign));
    
    //pop GVT
    if (fn_globals) {
      GPVar gvt_param(m_compiler.newGP());
      m_compiler.mov(gvt_param, imm((uint64_t)m_gvt));
      GPVar fn_id_param(m_compiler.newGP());
      m_compiler.mov(fn_id_param, imm(fn_id));
      
      
      ECall *ctx = m_compiler.call(imm((sysint_t)&popGVT));
      ctx->setPrototype(CALL_CONV_DEFAULT,
                        FunctionBuilder2<void, uint64_t, uint64_t>());
      ctx->setArgument(0, gvt_param);
      ctx->setArgument(1, fn_id_param);
    }

    return ctx_desc;
  }
  
  inline AAVI * initReturn(ECall *call_desc, VarType ret_type) {
    if (ret_type == VT_VOID) { return NULL; }
    AAVI *aavi = new AAVI(true, m_compiler, ret_type);
    call_desc->setReturn(*aavi->aj_var);
    return aavi;
  }
  
  //WORKAROUND for case if (...) { return ...; } else { return ...; }
  // variable in return close is not alloc after it
  // have no time to investigate AsmJit internals
  void rtrnSpillWA() {
    m_compiler.cmp(*m_gp_tmp, *m_gp_tmp);
    m_compiler.je(*m_ret_lbl);
  }
  
  inline void rtrn(EvalResult *er, VarType expected_type,
                   uint16_t fn_id, uint64_t fn_globals) {
    if (expected_type == VT_VOID || er == NULL) { return rtrnSpillWA(); }
    
    if (er->effectiveType() != expected_type) {
      (expected_type == VT_DOUBLE) ? toDbl(er) : toInt(er);
    }
    
    if (er->effectiveType() == VT_DOUBLE) {
      XMMVar &ret_var = *(XMMVar *)m_ret_var;
      if (er->type == VT_INVALID) {
        m_compiler.movq(ret_var, AS_XMM(er->data.var));
      } else {
        LOAD_CONST_TO_XMM(ret_var, er->data.d)
      }
    } else {
      GPVar &ret_var = *(GPVar *)m_ret_var;
      if (er->type == VT_INVALID) {
        m_compiler.mov(ret_var, AS_GP(er->data.var));
      } else {
        m_compiler.mov(ret_var, imm(er->data.i));
      }
    }
    rtrnSpillWA();
  }
  
//------------------------------------------------------------------------------
// Variable access

  inline void load(const AstVar * v) {
    AAVI *vi = (AAVI *)v->info();
    if (!vi->is_global) { return; }
    
    // load var from GVT
    GPVar gvt_param(m_compiler.newGP());
    m_compiler.mov(gvt_param, imm((uint64_t)m_gvt));
    GPVar fn_id_param(m_compiler.newGP());
    m_compiler.mov(fn_id_param, imm(vi->fn_id));
    GPVar var_id_param(m_compiler.newGP());
    m_compiler.mov(var_id_param, imm(vi->global_ind));
    
    AAVI *result = new AAVI(false, m_compiler, VT_INT);
    sysint_t addr = (sysint_t)&loadGlobalVar;
    ECall *ctx = m_compiler.call(imm(addr));

    ctx->setPrototype(CALL_CONV_DEFAULT,
      FunctionBuilder3<uint64_t, uint64_t, uint64_t, uint64_t>());
    ctx->setArgument(0, gvt_param);
    ctx->setArgument(1, fn_id_param);
    ctx->setArgument(2, var_id_param);
    ctx->setReturn(AS_GP(result));
    
    // init result as new variable state;
    // it's safe to drop var info since function
    // that own variable has already been generated by this momemnt
    result->is_global = true;
    result->global_ind = vi->global_ind;
    result->fn_id = vi->fn_id;
    
    if (v->type() == VT_DOUBLE) {
      XMMVar *dbl_var = new XMMVar(m_compiler.newXMM());
      m_compiler.movq(*dbl_var, AS_GP(result));
      delete result->aj_var;
      result->aj_var = dbl_var;
      result->type = VT_DOUBLE;
    }
    
    delete vi;
    ((AstVar *)v)->setInfo(result);
  }
  
  inline void store(const AstVar * v, EvalResult er) {
    AAVI *vi = (AAVI *)v->info();
    
    VarType dst_type = v->type();
    VarType src_type = er.effectiveType();
    if ((dst_type == VT_DOUBLE && src_type == VT_STRING) ||
        (dst_type == VT_STRING && src_type == VT_DOUBLE)) {
      logError("Can't assign str to dbl or vica vesa");
    }
    
    if (dst_type == VT_DOUBLE) {
      switch (er.type) {
        case VT_INT: case VT_DOUBLE: {
          double dbl = er.type == VT_DOUBLE ? er.data.d : (double)er.data.i;
          LOAD_CONST_TO_XMM(AS_XMM(vi), dbl)
          break;
        }
        case VT_STRING: assert(0 && "Str to Dbl can't be converted");
        case VT_INVALID: { // means variable
          AAVI *src_vi = (AAVI *)er.data.var;
          if (src_vi->type == VT_DOUBLE) {
            m_compiler.movq(AS_XMM(vi), AS_XMM(src_vi));
          } else {
            m_compiler.movq(AS_XMM(vi), AS_GP(src_vi));
            m_compiler.cvtdq2pd(AS_XMM(vi), AS_XMM(vi));
          }
          break;
        }
        default: assert(0 && "WTF type");
      }
    } else { // INT, STR
      GPVar &dest_var = AS_GP(vi);
      switch (er.type) {
        case VT_INT:   {m_compiler.mov(dest_var, er.data.i); break;}
        case VT_DOUBLE:{m_compiler.mov(dest_var, (uint64_t)er.data.d); break;}
        case VT_STRING:{m_compiler.mov(dest_var, (uint64_t)er.data.str); break;}
        case VT_INVALID: {
          AAVI *src_vi = (AAVI *)er.data.var;
          if (src_vi->type == VT_DOUBLE) {
            //use tmp, since we don't want to polute src
            XMMVar tmp(m_compiler.newXMM());
            m_compiler.cvtpd2dq(tmp, AS_XMM(src_vi));
            m_compiler.movq(dest_var, tmp);
            m_compiler.unuse(tmp);
          } else {
            m_compiler.mov(dest_var, AS_GP(src_vi));
          }
          break;
        }
        default: assert(0 && "WTF type");
      }
    }
    
    // Store to GVT
    updateGVT(vi);
  }
  
  inline void updateGVT(AAVI *vi) {
    if (vi->is_global) {
      GPVar gvt_param(m_compiler.newGP());
      m_compiler.mov(gvt_param, imm((uint64_t)m_gvt));
      GPVar fn_id_param(m_compiler.newGP());
      m_compiler.mov(fn_id_param, imm(vi->fn_id));
      GPVar var_id_param(m_compiler.newGP());
      m_compiler.mov(var_id_param, imm(vi->global_ind));
      
      GPVar var_value_param(m_compiler.newGP());
      if (vi->type == VT_DOUBLE) {
        m_compiler.movq(var_value_param, AS_XMM(vi));
      } else {
        m_compiler.mov(var_value_param, AS_GP(vi));
      }
      
      ECall *ctx = m_compiler.call(imm((sysint_t)&storeGlobalVar));
      ctx->setPrototype(CALL_CONV_DEFAULT,
                        FunctionBuilder4<void, uint64_t, uint64_t, uint64_t, uint64_t>());
      ctx->setArgument(0, gvt_param);
      ctx->setArgument(1, fn_id_param);
      ctx->setArgument(2, var_id_param);
      ctx->setArgument(3, var_value_param);
    }
  }
  
//------------------------------------------------------------------------------
// Statement generation
  
  inline void ifStmnt(EvalResult cond_er, std::vector<Emittable *> cond,
        std::vector<Emittable *> then_blk, std::vector<Emittable *> else_blk) {
    if (!cond_er.isConstant()) {
      AsmJit::Label else_lbl(m_compiler.newLabel());
    
      addEmmitablesToCode(cond);
      toInt(&cond_er);
      genCondCheck(cond_er, else_lbl);
      addEmmitablesToCode(then_blk);
    
      if (!else_blk.empty()) {
        AsmJit::Label eos_lbl(m_compiler.newLabel()); // end of statement label
        m_compiler.jmp(eos_lbl); // end of then
        
        m_compiler.bind(else_lbl);
        addEmmitablesToCode(else_blk);
        m_compiler.bind(eos_lbl);
      } else {
        m_compiler.bind(else_lbl);
      }
    } else {
      toInt(&cond_er);
      addEmmitablesToCode(cond_er.data.i == 0 ? else_blk : then_blk);
    }
  }
  
  inline void whileStmnt(EvalResult cond_er, std::vector<Emittable *> cond,
                         std::vector<Emittable *> body_blk) {
    if (!cond_er.isConstant()) {
      AsmJit::Label loop_end(m_compiler.newLabel());
      AsmJit::Label loop_start(m_compiler.newLabel());
    
      m_compiler.bind(loop_start);
      addEmmitablesToCode(cond);
      toInt(&cond_er);
      genCondCheck(cond_er, loop_end);
    
      addEmmitablesToCode(body_blk);
      m_compiler.jmp(loop_start);
      m_compiler.bind(loop_end);
    } else {
      toInt(&cond_er);
      if (cond_er.data.i != 0) { //infinite loop
        AsmJit::Label loop_start(m_compiler.newLabel());
        
        m_compiler.bind(loop_start);
        addEmmitablesToCode(body_blk);
        m_compiler.jmp(loop_start);
      }
    }
  }
  
  inline void forStmnt(
      const AstVar *ast_var, EvalResult init_er,
      std::vector<Emittable *> init_blk, EvalResult last_er,
      std::vector<Emittable *> last_blk, std::vector<Emittable *> body_blk) {

    AAVI *var = new AAVI(true, m_compiler, VT_INT);
    ERV erv; erv.var = var;
    EvalResult var_er = EvalResult(VT_INVALID, erv);
    
    addEmmitablesToCode(init_blk);
    toInt(&init_er);
    
    AsmJit::Label loop_end(m_compiler.newLabel());
    AsmJit::Label loop_start(m_compiler.newLabel());
    INT_PERFORM_OP(AS_GP(var), &init_er, mov)
    
    m_compiler.bind(loop_start);
    store(ast_var, var_er);
    
    addEmmitablesToCode(last_blk);
    toInt(&last_er);

    // cmp with last
    INT_PERFORM_OP(AS_GP(var), &last_er, cmp)
    m_compiler.jg(loop_end);
    
    addEmmitablesToCode(body_blk);

    ERV erv_one; erv_one.i = 1;
    EvalResult one_er = EvalResult(VT_INT, erv_one);
    add(&var_er, &one_er);
    
    m_compiler.jmp(loop_start);
    m_compiler.bind(loop_end);
    delete var;
  }
  
private: // methods
  
  inline int64_t d2i(double val) { return *(int64_t *)&val; }
  inline double i2d(int64_t val) { return *(double *)&val; }
  
  inline void genCondCheck(EvalResult cond_er, AsmJit::Label &lbl) {
    m_compiler.cmp(AS_GP(cond_er.data.var), imm(0));
    m_compiler.jz(lbl);
  }
  
  void copyContentToVar(AAVI *src, AAVI *dest) {
    if (src->type == VT_DOUBLE) {
      m_compiler.movq(AS_XMM(dest), AS_XMM(src));
    } else {
      m_compiler.mov(AS_GP(dest), AS_GP(src));
    }
  }
  
  inline std::string prnt_fmt_spec(VarType t) {
    switch (t) {
      case VT_DOUBLE: return "%g";
      case VT_INT: return "%lld";
      case VT_STRING: return "%s";
        
      default: assert(0 && "Unexpected type");
    }
  }
  
  inline EvalResult int_divide(EvalResult *a, EvalResult *b, bool rem_requred) {
    if (rem_requred) {
      TRY_INT_CONST_COMPUTE(a, %, b)
    } else {
      TRY_INT_CONST_COMPUTE(a, /, b)
    }

    INT_PERFORM_OP(AS_GP(m_divisor), b, mov)
    
    AAVI *div = new AAVI(true, m_compiler, VT_INT);
    INT_PERFORM_OP(AS_GP(div), a, mov)
    
    AAVI *rem = new AAVI(true, m_compiler, VT_INT);
    m_compiler.xor_(AS_GP(rem), AS_GP(rem));
    
    // use one common var as divisor as WORKAROUND for div late init bug
    m_compiler.idiv_lo_hi(AS_GP(div), AS_GP(rem), AS_GP(m_divisor));

    if (HAS_TRANSIENT_VAR(a)) { delete a->data.var; }
    if (HAS_TRANSIENT_VAR(b)) { delete b->data.var; }
    
    ERV erv; erv.var = rem_requred ? rem : div;
    return EvalResult(VT_INVALID, erv);
  }
  
  FunctionBuilderX convToFB(Signature sign) {
    #define UPDATE_FB(type)                                                    \
      (arg_ti == 0) ? fn_bldr.setReturnValue<type>() :                         \
                      fn_bldr.addArgument<type>();
    
    FunctionBuilderX fn_bldr;
    for (uint32_t arg_ti = 0; arg_ti < sign.size(); ++arg_ti) {
      switch (sign.at(arg_ti).first) {
        case VT_DOUBLE: { UPDATE_FB(double); break;   }
        case VT_INT:    { UPDATE_FB(uint64_t); break; }
        case VT_STRING: { UPDATE_FB(char *); break;   }
        case VT_VOID: {
          (arg_ti == 0) ? fn_bldr.setReturnValue<void>() :
                          assert(0 && "Void params are not allowed");
          break;
        }
        default: break;
      }
    }
    return fn_bldr;
    #undef UPDATE_FB
  }
  
  void addEmmitablesToCode(std::vector<Emittable *> asm_code) {
    for (size_t i = 0; i < asm_code.size(); ++i) {
      m_compiler.addEmittable(asm_code[i]);
    }
  }

  inline void logError(const char *error) {
    if (m_error_status) {  return; }
    m_error_status = new Status(error);
  }
  
private: //fields
  map<string, Instruction> m_name_to_instr;
  Status *m_error_status;
  EFunction * m_func;
  
  Compiler m_compiler;
  void *m_prnt_code;
  uint64_t *m_gvt;
  std::stack<Emittable *> m_marked_emitable;
  
  AAVI *m_divisor;
  AsmJit::Label *m_ret_lbl;
  BaseVar *m_ret_var;
  
  //GP func spec tmp var to make Asmit life easy
  GPVar *m_gp_tmp;
};

#undef AS_XMM
#undef AS_GP

#endif
