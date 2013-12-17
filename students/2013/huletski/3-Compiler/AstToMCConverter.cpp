//
//  AstToMCConverter.cpp
//  VM_3
//
//  Created by Hatless Fox on 12/11/13.
//  Copyright (c) 2013 WooHoo. All rights reserved.
//

#include "AstToMCConverter.h"

#include <dlfcn.h>
#include <vector>
#include <stdint.h>

#include "CommonStructs.h"
#include "AstToMCConverter.h"
#include "mathvm.h"

using namespace mathvm;

#define GEN_OPERATION(l, op, r) m_eval_results.push(m_isa.op(&l, &r))

void AstToMCConverter::handle_function_definition(AstFunction *func) {
  if (isFunctionNative(func)) { return; }
  
  m_curr_fn = (GeneratedMCF *)m_code->functionByName(func->name());
  
  m_curr_fn->generateAjFunc(&m_isa);
  m_isa.setFunction(m_curr_fn->asmjitFunction());

  if (func->name() == "<top>") {
    m_isa.performTopGVTPush(m_curr_fn->id(), m_curr_fn->globals());
  }
  
  Scope *scope = func->node()->body()->scope();
  //init infos of AstVars that corresponds to parameters
  for (uint32_t i = 0; i < func->parametersNumber(); ++i) {
    AstVar *var = scope->lookupVariable(func->parameterName(i));
    FPVI *v_i = (FPVI *)var->info();
    var->setInfo(m_isa.initArg(i, var->type(), v_i->func_id));
  }
  
  func->node()->visit(this);
  
  m_isa.finalizeFuncProcessing(m_curr_fn->returnType());
  m_isa.setFunction(NULL);
  
  if (!m_err_status && m_isa.errorStatus()) {
    m_err_status = m_isa.errorStatus();
  }
}

//------------------------------------------------------------------------------
// Trivial Nodes

void AstToMCConverter::visitNativeCallNode(NativeCallNode* node) {
  AstBaseVisitor::visitNativeCallNode(node);
}

void AstToMCConverter::visitFunctionNode(mathvm::FunctionNode* node) {
  AstBaseVisitor::visitFunctionNode(node);
}

//------------------------------------------------------------------------------
// Literals handling

void AstToMCConverter::visitStringLiteralNode(StringLiteralNode* node) {
  if (!m_active_assigments) { return; }
  
  EvalResulValue erv; erv.str = (char *)node->literal().c_str();
  m_eval_results.push(EvalResult(VT_STRING, erv));
}

void AstToMCConverter::visitDoubleLiteralNode(DoubleLiteralNode* node) {
  if (!m_active_assigments) { return; }
  
  EvalResulValue erv;
  erv.d = node->literal();
  m_eval_results.push(EvalResult(VT_DOUBLE, erv));
}

void AstToMCConverter::visitIntLiteralNode(IntLiteralNode* node) {
  if (!m_active_assigments) { return; }
  
  EvalResulValue erv; erv.i = node->literal();
  m_eval_results.push(EvalResult(VT_INT, erv));
}

//------------------------------------------------------------------------------
// Build-in Commands handling

void AstToMCConverter::visitPrintNode(PrintNode* node) {
  std::vector<EvalResult> print_args;
  
  ++m_active_assigments;
  for (uint32_t i = 0; i < node->operands(); i++) {
    node->operandAt(i)->visit(this);
    print_args.push_back(recent_eval_result());
  }
  --m_active_assigments;
  
  m_isa.print(print_args, m_code);
}

//------------------------------------------------------------------------------
// Variable access

void AstToMCConverter::visitLoadNode(LoadNode* node) {
  if (!m_active_assigments) { return; }
  
  ERV erv; erv.var = cloneAAVI(node->var());
  m_eval_results.push(EvalResult(VT_INVALID, erv));
  m_isa.load(node->var());
}

void AstToMCConverter::visitStoreNode(StoreNode* node) {
  ++m_active_assigments;
  node->value()->visit(this);
  --m_active_assigments;
  
  const AstVar *dest_var = node->var();
  if (node->op() != tASSIGN) {
    EvalResult modifier = recent_eval_result();
    m_isa.load(dest_var);
    ERV erv; erv.var = cloneAAVI(dest_var);
    EvalResult loaded(VT_INVALID, erv);
    
    if (node->op() == tINCRSET) { GEN_OPERATION(loaded, add, modifier); }
    else {                        GEN_OPERATION(loaded, sub, modifier); }
  }
  m_isa.store(dest_var, recent_eval_result());
}

//------------------------------------------------------------------------------
// Function calls

void AstToMCConverter::visitCallNode(CallNode* node) {
  GeneratedMCF *func = (GeneratedMCF *)m_code->functionByName(node->name());
  
  if (!func) {
    logError("Wrong call func name: " + node->name());
    return;
  }
  
  std::vector<EvalResult> args;
  ++m_active_assigments;
  for (uint32_t i = 0; i < node->parametersNumber(); i++) {
    node->parameterAt(i)->visit(this);
    EvalResult er = recent_eval_result();
    m_isa.ensureInVar(&er);
    VarType expected_type = func->signature()[i + 1].first;
    VarType eval_type = er.effectiveType();
    if (expected_type != eval_type) {
      if (eval_type == VT_INT && expected_type == VT_DOUBLE) {
        m_isa.toDbl(&er);
      } else if (eval_type == VT_DOUBLE && expected_type == VT_INT) {
        m_isa.toInt(&er);
      } else if ((eval_type == VT_INT && expected_type == VT_STRING) ||
                 (eval_type == VT_STRING && expected_type == VT_INT)){
      } else {
        logError("Unsupported conversion during " + node->name() + " call");
      }
    }
    
    args.push_back(er);
  }
  --m_active_assigments;
  
  ECall *call_desc = m_isa.call(func->signature(), func->id(), func->globals());
  func->initECall(call_desc);
  for (uint32_t i = 0; i < args.size(); i++) {
    call_desc->setArgument(i, *(args[i].data.var->aj_var));
  }
  
  AAVI * aavi= m_isa.initReturn(call_desc, func->returnType());
  if (aavi) {
    ERV erv; erv.var = aavi;
    m_eval_results.push(EvalResult(VT_INVALID, erv));
  }
}

void AstToMCConverter::visitReturnNode(ReturnNode* node) {
  VarType return_type = m_curr_fn->returnType();
  if (node->returnExpr()) {
    ++m_active_assigments;
    node->returnExpr()->visit(this);
    --m_active_assigments;
    EvalResult res = recent_eval_result();
    m_isa.rtrn(&res, return_type, m_curr_fn->id(), m_curr_fn->globals());
  } else {
    m_isa.rtrn(NULL, return_type, m_curr_fn->id(), m_curr_fn->globals());
  }
}

//------------------------------------------------------------------------------
// Operations

void AstToMCConverter::visitBinaryOpNode(BinaryOpNode* node) {
  if (m_active_assigments == 0) { return; }
  
  node->left()->visit(this);
  EvalResult left_er = recent_eval_result();
  node->right()->visit(this);
  EvalResult right_er = recent_eval_result();
  
  TokenKind op = node->kind();
  switch (op) {
    case tOR: case tAND:
    case tMOD:
    case tAOR: case tAAND: case tAXOR: {
      m_isa.toInts(&left_er, &right_er);
      switch (op) {
        case tOR:  GEN_OPERATION(left_er, lor, right_er); break;
        case tAND: GEN_OPERATION(left_er, land, right_er); break;
        case tMOD:  GEN_OPERATION(left_er, mod, right_er); break;
          
        case tAOR:  GEN_OPERATION(left_er, aor, right_er); break;
        case tAAND: GEN_OPERATION(left_er, aand, right_er); break;
        case tAXOR: GEN_OPERATION(left_er, axor, right_er); break;
          
        default: assert(0); //programmer error
      }
      break;
    }
      
    case tEQ: case tNEQ: case tGE: case tGT: case tLE: case tLT:
    case tADD: case tSUB: case tMUL: case tDIV: {
      m_isa.toBroader(&left_er, &right_er);
      switch (op) {
        case tEQ:  GEN_OPERATION(left_er, leq, right_er); break;
        case tNEQ: GEN_OPERATION(left_er, lne, right_er); break;
        case tGE:  GEN_OPERATION(left_er, lge, right_er); break;
        case tGT:  GEN_OPERATION(left_er, lg, right_er); break;
        case tLE:  GEN_OPERATION(left_er, lle, right_er); break;
        case tLT:  GEN_OPERATION(left_er, ll, right_er); break;
          
        case tADD: GEN_OPERATION(left_er, add, right_er); break;
        case tMUL: GEN_OPERATION(left_er, mul, right_er); break;
        case tSUB: GEN_OPERATION(left_er, sub, right_er); break;
        case tDIV: GEN_OPERATION(left_er, div, right_er); break;
          
        default: assert(0); //programmer error
      }
      break;
    }
    case tINCRSET: case tDECRSET: {
      op == tINCRSET ? GEN_OPERATION(left_er, add, right_er) :
      GEN_OPERATION(left_er, sub, right_er);
      
      if (!node->left()->isLoadNode()) {
        logError("Unexpected left value in binary assignement");
        return;
      }
      
      const AstVar *var = node->left()->asLoadNode()->var();
      m_isa.store(var, recent_eval_result());
      visitLoadNode(node->left()->asLoadNode());
      break;
    }
      
    case tRANGE:break; // FIXME: ignore, handle it during for node visiting
    default: logError("Unknown instruction"); break;
  }
}

void AstToMCConverter::visitUnaryOpNode(UnaryOpNode* node) {
  if (!m_active_assigments) { return; }
  
  node->operand()->visit(this);
  EvalResult er = recent_eval_result();
  
  switch (node->kind()) {
    case tSUB: m_isa.neg(&er); break;
    case tNOT: m_isa.nt(&er); break;
    default: logError("Unknown insnruction"); break;
  }
  m_eval_results.push(er);
}

//------------------------------------------------------------------------------
// Language structures

void AstToMCConverter::visitBlockNode(mathvm::BlockNode* node) {
  Scope::VarIterator vi(node->scope());
  while (vi.hasNext()) {
    AstVar *var = vi.next();
    var->setInfo(m_isa.asmVarInfo(var->type(), (FPVI *)var->info()));
  }
  AstBaseVisitor::visitBlockNode(node);
}

void AstToMCConverter::visitIfNode(IfNode* node) {
  std::vector<Emittable *> cond_blk;
  std::vector<Emittable *> then_blk;
  std::vector<Emittable *> else_blk;
  
  ++m_active_assigments;
  m_isa.mark();
  node->ifExpr()->visit(this);
  EvalResult cond_er = recent_eval_result();
  cond_blk = m_isa.restore();
  --m_active_assigments;
  
  m_isa.mark();
  node->thenBlock()->visit(this);
  then_blk = m_isa.restore();
  
  if (node->elseBlock()) {
    m_isa.mark();
    node->elseBlock()->visit(this);
    else_blk = m_isa.restore();
  }
  
  m_isa.ifStmnt(cond_er, cond_blk, then_blk, else_blk);
}

void AstToMCConverter::visitWhileNode(WhileNode* node) {
  std::vector<Emittable *> cond_blk;
  std::vector<Emittable *> body_blk;
  
  ++m_active_assigments;
  m_isa.mark();
  node->whileExpr()->visit(this);
  EvalResult cond_er = recent_eval_result();
  cond_blk = m_isa.restore();
  --m_active_assigments;
  
  m_isa.mark();
  node->loopBlock()->visit(this);
  body_blk = m_isa.restore();
  
  m_isa.whileStmnt(cond_er, cond_blk, body_blk);
}

void AstToMCConverter::visitForNode(ForNode* node) {
  if (!node->inExpr()->isBinaryOpNode() ||
      node->inExpr()->asBinaryOpNode()->kind() != tRANGE) {
    logError("For node have unexpected structure");
    return;
  }
  
  std::vector<Emittable *> init_blk;
  std::vector<Emittable *> last_blk;
  std::vector<Emittable *> body_blk;
  
  BinaryOpNode *bon = node->inExpr()->asBinaryOpNode();
  
  ++m_active_assigments;
  m_isa.mark();
  bon->left()->visit(this);
  init_blk = m_isa.restore();
  EvalResult init_er = recent_eval_result();
  
  m_isa.mark();
  bon->right()->visit(this);
  last_blk = m_isa.restore();
  EvalResult last_er = recent_eval_result();
  --m_active_assigments;
  
  m_isa.mark();
  node->body()->visit(this);
  body_blk = m_isa.restore();
  
  m_isa.forStmnt(node->var(), init_er, init_blk, last_er, last_blk, body_blk);
}

#undef GEN_OPERATION