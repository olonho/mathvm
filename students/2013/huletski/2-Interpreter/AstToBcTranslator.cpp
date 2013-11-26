//
//  AstToBCTranslator.cpp
//  VM_2
//
//  Created by Hatless Fox on 10/25/13.
//  Copyright (c) 2013 WooHoo. All rights reserved.
//

#include "AstToBcTranslator.h"
#include "mathvm.h"
#include "CommonStructs.h"

using namespace mathvm;


void AstToBCTranslator::handle_function_definition(AstFunction *func) {
  BytecodeFunction *curr_func = (BytecodeFunction *) m_code->functionByName(func->name());
  
  m_curr_funcs.push(curr_func);
  m_isa.setBC(curr_func->bytecode());
  
  curr_func->setScopeId(m_scopes.size());
  
  Scope *scope = func->node()->body()->scope();
  m_scopes.push_back(scope);
  curr_func->setLocalsNumber(1 + func->parametersNumber());

  uint16_t assigned_locals = 1; // zero loral is reserved
  uint16_t scope_id = m_curr_funcs.top()->scopeId();
  //init infos of AstVars that corresponds to parameters
  for (uint32_t i = 0; i < func->parametersNumber(); ++i) {
    AstVar * param = scope->lookupVariable(func->parameterName(i));
    param->setInfo(new VarInfo(assigned_locals++, scope_id, param->type()));
  }
  
  func->node()->visit(this);
  
  //std::cout << "==== " << func->name() << std::endl;
  //m_curr_funcs.top()->bytecode()->dump(std::cout);
  m_curr_funcs.pop();
  m_isa.setBC(m_curr_funcs.size() ? m_curr_funcs.top()->bytecode() : NULL);
  
  if (!m_err_status && m_isa.errorStatus()) {
    m_err_status = m_isa.errorStatus();
  }
}

void AstToBCTranslator::visitBlockNode(BlockNode* node) {
  uint16_t scope_id = m_curr_funcs.top()->scopeId();
  uint16_t assigned_locals = m_curr_funcs.top()->localsNumber();
  Scope::VarIterator vi(node->scope());
  while (vi.hasNext()) {
    //assign to each variable a unique location
    AstVar *var = vi.next();

    var->setInfo(new VarInfo(assigned_locals++, scope_id, var->type()));
  }
  m_curr_funcs.top()->setLocalsNumber(assigned_locals);

  //emulate forward declaration
  Scope::FunctionIterator fi(node->scope());
  while (fi.hasNext()) { registerFunction(fi.next()); }
  
  // NB: function processing should be done after vars since funcs may use vars
  fi = node->scope();
  while (fi.hasNext()) { handle_function_definition(fi.next()); }

  AstBaseVisitor::visitBlockNode(node);
}

void AstToBCTranslator::visitFunctionNode(FunctionNode *node) {
  node->visitChildren(this); //Do nothing
}

void AstToBCTranslator::visitPrintNode(PrintNode* node) {
  ++m_active_assigments;
  for (uint32_t i = 0; i < node->operands(); i++) {
    node->operandAt(i)->visit(this);
    m_isa.print(tos_type());
    m_tos_types.pop();
  }
  --m_active_assigments;
}

void AstToBCTranslator::visitLoadNode(LoadNode* node) {
  if (m_active_assigments == 0) { return; }
  
  VarInfo *v_i = (VarInfo *)node->var()->info();
  VarType v_type = node->var()->type();
  
  //assert(v_i->scope_id <= m_curr_funcs.top()->scopeId()); // programmer error
  m_isa.load(v_i, m_curr_funcs.top()->scopeId());
  m_tos_types.push(v_type);
}


void AstToBCTranslator::visitStoreNode(StoreNode* node) {
  ++m_active_assigments;
  node->value()->visit(this);
  --m_active_assigments;
  
  VarInfo *v_i = (VarInfo *)node->var()->info();
  VarType v_type = node->var()->type();
  uint16_t scope_id = m_curr_funcs.top()->scopeId();
  
  m_isa.convert(tos_type(), v_type);
  if (node->op() != tASSIGN) {
    m_isa.load(v_i, scope_id);
    node->op() == tINCRSET ? m_isa.add(v_type) : m_isa.sub(v_type);
  }
  m_isa.store(v_i, scope_id);
  m_tos_types.pop();
}

void AstToBCTranslator::visitReturnNode(ReturnNode* node) {
  if (node->returnExpr()) {
    ++m_active_assigments;
    node->returnExpr()->visit(this);
    --m_active_assigments;
  
    VarType ret_type = m_curr_funcs.top()->returnType();
    m_isa.convert(tos_type(), ret_type);
    m_tos_types.pop();
  }
  
  m_isa.rtrn();
}

void AstToBCTranslator::visitBinaryOpNode(BinaryOpNode* node) {
  if (m_active_assigments == 0) { return; }
  
  VarType new_tos_type = VT_INVALID;
  node->right()->visit(this);
  VarType r_type = tos_type();
  node->left()->visit(this);
  VarType l_type = tos_type();
  
  TokenKind op = node->kind();

  switch (op) {
    case tOR: case tAND:
    case tMOD:
    case tAOR: case tAAND: case tAXOR: {
      new_tos_type = m_isa.toInts(l_type, r_type);
      switch (op) {
        case tOR: m_isa.lor(); break;
        case tAND: m_isa.land(); break;
        case tMOD: m_isa.mod(VT_INT); break;
        
        case tAOR: m_isa.aor(); break;
        case tAAND: m_isa.aand(); break;
        case tAXOR: m_isa.axor(); break;
          
        default: assert(0); //programmer error
      }
      break;
    }
      
    case tEQ: case tNEQ: case tGE: case tGT: case tLE: case tLT:
    case tADD: case tSUB: case tMUL: case tDIV: {
      new_tos_type = m_isa.toBroader(l_type, r_type);
      switch (op) {
        case tEQ: m_isa.leq(new_tos_type); break;
        case tNEQ: m_isa.lneq(new_tos_type); break;
        case tGE: m_isa.lge(new_tos_type); break;
        case tGT: m_isa.lgt(new_tos_type); break;
        case tLE: m_isa.lle(new_tos_type); break;
        case tLT: m_isa.llt(new_tos_type); break;
          
        case tADD: m_isa.add(new_tos_type); break;
        case tSUB: m_isa.sub(new_tos_type); break;
        case tMUL: m_isa.mul(new_tos_type); break;
        case tDIV: m_isa.div(new_tos_type); break;

        default: assert(0); //programmer error
      }
      break;
    }
      
    case tINCRSET: case tDECRSET: {
      new_tos_type = m_isa.toBroader(l_type, r_type);
      op == tINCRSET ? m_isa.add(new_tos_type) : m_isa.sub(new_tos_type);
      
      if (!node->left()->isLoadNode()) {
        logError("Unexpected left value in binary assignement");
        return;
      }
      
      const AstVar *var = node->left()->asLoadNode()->var();
      VarInfo *v_i = (VarInfo *)var->info();
      uint16_t scope_id = m_curr_funcs.top()->scopeId();
      
      m_isa.store(v_i, scope_id);
      m_isa.load(v_i, scope_id); //need to load since it's is part of expression
      break;
    }
      
     
    case tRANGE:break; // FIXME: ignore, handle it during for node visiting
    default: logError("Unknown binary instruction"); break;
  }
  
  m_tos_types.pop();
  m_tos_types.pop();
  m_tos_types.push(new_tos_type);
}

void AstToBCTranslator::visitUnaryOpNode(UnaryOpNode* node) {
  if (!m_active_assigments) { return; }
  
  node->operand()->visit(this);

  if (node->kind() == tSUB) {
    m_isa.neg(tos_type());
  } else if (node->kind() == tNOT){
    m_isa.nt(tos_type());
  }
}

#pragma mark - Literal Translation

void AstToBCTranslator::visitStringLiteralNode(StringLiteralNode* node) {
  if (!m_active_assigments) { return; }
  
  m_isa.pushStr(m_code->makeStringConstant(node->literal()));
  m_tos_types.push(VT_STRING);
}

void AstToBCTranslator::visitDoubleLiteralNode(DoubleLiteralNode* node) {
  if (!m_active_assigments) { return; }
  
  m_isa.pushDouble(node->literal());
  m_tos_types.push(VT_DOUBLE);
}

void AstToBCTranslator::visitIntLiteralNode(IntLiteralNode* node) {
  if (!m_active_assigments) { return; }
  
  m_isa.pushInt(node->literal());
  m_tos_types.push(VT_INT);
}

#pragma mark - Control Structures Translation

void AstToBCTranslator::visitIfNode(IfNode* node) {
  Bytecode cond_blk;
  Bytecode then_blk;
  Bytecode else_blk;
  Bytecode *tmp_bc = m_isa.bc();
  
  ++m_active_assigments;
  m_isa.setBC(&cond_blk);
  node->ifExpr()->visit(this);
  VarType cond_tos_type = tos_type();
  m_tos_types.pop();
  --m_active_assigments;
  
  m_isa.setBC(&then_blk);
  node->thenBlock()->visit(this);
  
  if (node->elseBlock()) {
    m_isa.setBC(&else_blk);
    node->elseBlock()->visit(this);
  }
  
  m_isa.setBC(tmp_bc);
  m_isa.ifStmnt(cond_tos_type, &cond_blk, &then_blk, &else_blk);
}

void AstToBCTranslator::visitWhileNode(WhileNode* node) {
  Bytecode cond_blk;
  Bytecode body_blk;
  Bytecode *tmp_bc = m_isa.bc();
  
  ++m_active_assigments;
  m_isa.setBC(&cond_blk);
  node->whileExpr()->visit(this);
  VarType cond_tos_type = tos_type();
  m_tos_types.pop();
  --m_active_assigments;
  
  m_isa.setBC(&body_blk);
  node->loopBlock()->visit(this);
  
  m_isa.setBC(tmp_bc);
  m_isa.whileStmnt(cond_tos_type, &cond_blk, &body_blk);
}

void AstToBCTranslator::visitForNode(ForNode* node) {
  if (!node->inExpr()->isBinaryOpNode() ||
      node->inExpr()->asBinaryOpNode()->kind() != tRANGE) {
    logError("For node have unexpected structure");
    return;
  }
 
  Bytecode init_val;
  Bytecode last_val;
  Bytecode body_blk;
  Bytecode *tmp_bc = m_isa.bc();
  
  BinaryOpNode *bon = node->inExpr()->asBinaryOpNode();
  ++m_active_assigments;
  m_isa.setBC(&init_val);
  bon->left()->visit(this);
  m_tos_types.pop();
  m_isa.setBC(&last_val);
  bon->right()->visit(this);
  m_tos_types.pop();
  --m_active_assigments;
  
  m_isa.setBC(&body_blk);
  node->body()->visit(this);
  
  m_isa.setBC(tmp_bc);
  m_isa.forStmnt((VarInfo *)node->var()->info(),
                 m_curr_funcs.top()->scopeId(),
                 &init_val, &last_val, &body_blk);
}

#pragma mark - Func Calls Translation

void AstToBCTranslator::visitCallNode(CallNode* node) {
  // load params to TOS
  ++m_active_assigments;
  for (uint32_t i = 0; i < node->parametersNumber(); i++) {
    node->parameterAt(i)->visit(this);
    m_isa.convert(tos_type(), m_code->functionByName(node->name())->parameterType(i));
    m_tos_types.pop();
  }
  --m_active_assigments;
  
  m_isa.call();
  
  m_tos_types.push(m_code->functionByName(node->name())->returnType());
  Scope *nearest_scope = m_scopes[m_curr_funcs.top()->scopeId()];
  uint64_t func_addr = (uint64_t)nearest_scope->lookupFunction(node->name());
  m_isa.addUInt16(getRegisteredFuncId(func_addr));
}

void AstToBCTranslator::visitNativeCallNode(NativeCallNode* node) {
  //TODO: implement me!
}
