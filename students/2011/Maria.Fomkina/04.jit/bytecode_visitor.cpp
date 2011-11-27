#include "bytecode_visitor.h"
#include <cstdio>
#include <cstdlib>
#include "mvm_code.h"
#include "mathvm.h"

namespace mathvm {
  
void compare_operation(Bytecode *bcode_, TokenKind kind,
                       VarType var_type) {
  Label is_true(bcode_), next(bcode_);
  if (var_type == VT_DOUBLE) {
    bcode_->add(BC_SWAP);
    bcode_->add(BC_D2I);
    bcode_->add(BC_SWAP);
    bcode_->add(BC_D2I);
  }
  switch (kind) {
    case tEQ: { bcode_->addBranch(BC_IFICMPE, is_true); break; }
    case tNEQ: { bcode_->addBranch(BC_IFICMPNE, is_true); break; }
    case tGT: { bcode_->addBranch(BC_IFICMPG, is_true); break; }
    case tGE: { bcode_->addBranch(BC_IFICMPGE, is_true); break; }
    case tLT: { bcode_->addBranch(BC_IFICMPL, is_true); break; }
    case tLE: { bcode_->addBranch(BC_IFICMPLE, is_true); break; }
    default: { break; }
  }
  bcode_->add(BC_ILOAD0);
  bcode_->addBranch(BC_JA, next);
  bcode_->bind(is_true);
  bcode_->add(BC_ILOAD1);
  bcode_->bind(next);
}

void logic_operation(Bytecode *bcode_, TokenKind kind,
                     VarType var_type) {
  Label check_right(bcode_), final(bcode_), next(bcode_);
  if (var_type == VT_DOUBLE) {
    bcode_->add(BC_SWAP);
    bcode_->add(BC_D2I);
    bcode_->add(BC_SWAP);
    bcode_->add(BC_D2I);
  }
  if (kind == tOR) {
    bcode_->add(BC_ILOAD0);
    bcode_->addBranch(BC_IFICMPE, check_right);
    bcode_->add(BC_POP);
    bcode_->add(BC_ILOAD1);
    bcode_->bind(check_right);
    bcode_->add(BC_ILOAD0);
    bcode_->addBranch(BC_IFICMPE, final);
    bcode_->add(BC_ILOAD1);
    bcode_->addBranch(BC_JA, next);
    bcode_->bind(final);
    bcode_->add(BC_ILOAD0);
    bcode_->bind(next);
  }
  if (kind == tAND) {
    bcode_->add(BC_ILOAD0);
    bcode_->addBranch(BC_IFICMPNE, check_right);
    bcode_->add(BC_POP);
    bcode_->add(BC_ILOAD0);
    bcode_->bind(check_right);
    bcode_->add(BC_ILOAD0);
    bcode_->addBranch(BC_IFICMPNE, final);
    bcode_->add(BC_ILOAD0);
    bcode_->addBranch(BC_JA, next);
    bcode_->bind(final);
    bcode_->add(BC_ILOAD1);
    bcode_->bind(next);
  }
}

// ******************************************************** //
void BytecodeVisitor::visitBinaryOpNode(BinaryOpNode* node) {
  node->left()->visit(this);
  node->right()->visit(this);
  bcode_->add(BC_SWAP);
  switch (last_var_type_) {
    case VT_INT: {
      switch (node->kind()) {
        case tOR: case tAND: {
          logic_operation(bcode_, node->kind(), last_var_type_);
          break;
        }
        case tEQ: case tNEQ: case tGT: 
        case tGE: case tLT:  case tLE: { 
          compare_operation(bcode_, node->kind(), last_var_type_); 
          break; 
        }
        case tADD: { bcode_->add(BC_IADD); break; }
        case tSUB: { bcode_->add(BC_ISUB); break; }
        case tMUL: { bcode_->add(BC_IMUL); break; }
        case tDIV: { bcode_->add(BC_IDIV); break; }
        default: { break; }
      }
      break;
    }
    case VT_DOUBLE: {
      switch (node->kind()) {
        case tOR: case tAND: {
          logic_operation(bcode_, node->kind(), last_var_type_);
          break;
        }
        case tEQ: case tNEQ: case tGT: 
        case tGE: case tLT:  case tLE: { 
          compare_operation(bcode_, node->kind(), last_var_type_); 
          break; 
        }
        case tADD: { bcode_->add(BC_DADD); break; }
        case tSUB: { bcode_->add(BC_DSUB); break; }
        case tMUL: { bcode_->add(BC_DMUL); break; }
        case tDIV: { bcode_->add(BC_DDIV); break; }
        default: { break; }
      }
      break;
    }
    default: { break; }
  }
}

// ****************************************************** //
void BytecodeVisitor::visitUnaryOpNode(UnaryOpNode* node) {
  node->operand()->visit(this);
  switch (node->kind()) {
    case tNOT: {
      bcode_->add(BC_ILOAD1);
      bcode_->add(BC_ISUB);
      break;
    }
    case tSUB: {
      if (last_var_type_ == VT_DOUBLE) {
        bcode_->add(BC_DNEG);
      } else {
        bcode_->add(BC_INEG);
      }
      break;
    }
    default: { break; }
  }
}

// **************************************** //
void BytecodeVisitor::visitStringLiteralNode(
    StringLiteralNode* node) {
  bcode_->add(BC_SLOAD);
  bcode_->addInt16(code_->makeStringConstant(node->literal()));
  last_var_type_ = VT_STRING;
}

// **************************************** //-
void BytecodeVisitor::visitDoubleLiteralNode(
    DoubleLiteralNode* node) {
  bcode_->add(BC_DLOAD);
  bcode_->addDouble(node->literal());
  last_var_type_ = VT_DOUBLE;
}

// ************************************* //
void BytecodeVisitor::visitIntLiteralNode(
    IntLiteralNode* node) {
   bcode_->add(BC_ILOAD);
  bcode_->addInt64(node->literal());
  last_var_type_ = VT_INT;
}

// ************************************************ //
void BytecodeVisitor::visitLoadNode(LoadNode* node) {
  switch (node->var()->type()) {
    case VT_INVALID: break;
    case VT_VOID: break;
    case VT_DOUBLE: { bcode_->add(BC_LOADDVAR); break;}
    case VT_INT: { bcode_->add(BC_LOADIVAR); break;}
    case VT_STRING: { bcode_->add(BC_LOADSVAR); break;}
  }
  bcode_->addInt16(var_map_.GetVarId(node->var()->name()));
  last_var_type_ = node->var()->type();
}

// ************************************************** //
void BytecodeVisitor::visitStoreNode(StoreNode* node) {
  switch (node->var()->type()) { 
    case VT_INVALID: break;
    case VT_VOID: break;
    case VT_DOUBLE: {
      node->value()->visit(this);
      switch (node->op()) {
        case tINCRSET: {
          bcode_->add(BC_LOADDVAR); 
          bcode_->addInt16(var_map_.GetVarId(node->var()->name()));
          bcode_->add(BC_DADD);
          break;
        }
        case tDECRSET: {
          bcode_->add(BC_LOADDVAR); 
          bcode_->addInt16(var_map_.GetVarId(node->var()->name()));
          bcode_->add(BC_DSUB);
          break;
        }
      default: {}
      }      
      bcode_->add(BC_STOREDVAR); 
      bcode_->addInt16(var_map_.GetVarId(node->var()->name()));
      break;
    }
    case VT_INT: { 
      node->value()->visit(this);
      switch (node->op()) {
        case tINCRSET: {
          bcode_->add(BC_LOADIVAR); 
          bcode_->addInt16(var_map_.GetVarId(node->var()->name()));
          bcode_->add(BC_IADD);
          break;
        }
        case tDECRSET: {
          bcode_->add(BC_LOADIVAR); 
          bcode_->addInt16(var_map_.GetVarId(node->var()->name()));
          bcode_->add(BC_ISUB);
          break;
        }
        default: {}
      }
      bcode_->add(BC_STOREIVAR); 
      bcode_->addInt16(var_map_.GetVarId(node->var()->name()));
      break;
    }
    case VT_STRING: {
      node->value()->visit(this);
      bcode_->add(BC_STORESVAR); 
      bcode_->addInt16(var_map_.GetVarId(node->var()->name()));
      break;
    }
    default: {}
  }
}

// ********************************************** //
void BytecodeVisitor::visitForNode(ForNode* node) {
  var_map_.AddScope();
  var_map_.AddVar(node->var()->name());
  node->inExpr()->asBinaryOpNode()->left()->visit(this);
  bcode_->add(BC_STOREIVAR);
  bcode_->addInt16(var_map_.GetVarId(node->var()->name()));
  Label forBegin(bcode_);
  bcode_->bind(forBegin);
  node->body()->visit(this);
  node->inExpr()->asBinaryOpNode()->right()->visit(this);
  bcode_->add(BC_LOADIVAR);
  bcode_->addInt16(var_map_.GetVarId(node->var()->name()));
  bcode_->add(BC_ILOAD1);
  bcode_->add(BC_IADD);
  bcode_->add(BC_STOREIVAR);
  bcode_->addInt16(var_map_.GetVarId(node->var()->name()));
  bcode_->add(BC_LOADIVAR);
  bcode_->addInt16(var_map_.GetVarId(node->var()->name()));
  bcode_->addBranch(BC_IFICMPLE, forBegin);
  var_map_.DeleteScope();    
}

// ************************************************** //
void BytecodeVisitor::visitWhileNode(WhileNode* node) {
  Label whileBegin(bcode_), whileEnd(bcode_);
  bcode_->bind(whileBegin);
  node->whileExpr()->visit(this);
  bcode_->add(BC_ILOAD0);
  bcode_->addBranch(BC_IFICMPE, whileEnd);
  node->loopBlock()->visit(this);
  bcode_->addBranch(BC_JA, whileBegin);
  bcode_->bind(whileEnd);
}

// ******************************************** //
void BytecodeVisitor::visitIfNode(IfNode* node) {
  Label elseBranch(bcode_), ifEnd(bcode_);
  node->ifExpr()->visit(this);
  bcode_->add(BC_ILOAD0);
  bcode_->addBranch(BC_IFICMPE, elseBranch); 
  node->thenBlock()->visit(this);  
  bcode_->addBranch(BC_JA, ifEnd); 
  bcode_->bind(elseBranch);
  if (node->elseBlock() != NULL) node->elseBlock()->visit(this);
  bcode_->bind(ifEnd);
}

// ************************************************** //
void BytecodeVisitor::visitBlockNode(BlockNode* node) {
  Scope* scope = node->scope();
  Scope::VarIterator* it = new Scope::VarIterator(scope);
  var_map_.AddScope();
  while (it->hasNext()) {
    AstVar* var = it->next();
    var_map_.AddVar(var->name());
    switch (var->type()) { 
      case VT_DOUBLE: {
        bcode_->add(BC_DLOAD0);
        bcode_->add(BC_STOREDVAR); 
        bcode_->addInt16(var_map_.GetVarId(var->name()));
        break;
      }
      case VT_INT: {
        bcode_->add(BC_ILOAD0); 
        bcode_->add(BC_STOREIVAR); 
        bcode_->addInt16(var_map_.GetVarId(var->name()));
        break;
      }
      case VT_STRING: {
        bcode_->add(BC_SLOAD0); 
        bcode_->add(BC_STORESVAR); 
        bcode_->addInt16(var_map_.GetVarId(var->name()));
        break;
      }
      default: {}
    }
  }
  Scope::FunctionIterator *fit = new Scope::FunctionIterator(scope);
  while (fit->hasNext()) {
    FunctionNode* func = fit->next()->node();
    func->visit(this);
  }
  node->visitChildren(this);
  var_map_.DeleteScope();
}

// ************************************************** //
void BytecodeVisitor::visitPrintNode(PrintNode* node) {
  for (uint32_t i = 0; i < node->operands(); i++) {
    node->operandAt(i)->visit(this);
    switch (last_var_type_) {
      case VT_DOUBLE: { bcode_->add(BC_DPRINT); break; }
      case VT_INT: { bcode_->add(BC_IPRINT); break; }
      case VT_STRING: { bcode_->add(BC_SPRINT); break; }
      default: { break; }
    }
    bcode_->add(BC_POP);
  }
}

void BytecodeVisitor::visitFunctionNode(FunctionNode* node) {
  uint16_t id = ((MvmCode*)code_)->_functionids.size();
  ((MvmCode*)code_)->_functionids.insert(std::make_pair(node->name(), id));  
  BytecodeFunction* func = new BytecodeFunction(
      new AstFunction(node, node->body()->scope()));
  code_->addFunction(func);
  Bytecode* temp = bcode_;
  bcode_ = ((BytecodeFunction *)code_->functionById(id))->bytecode();
  var_map_.AddScope();
  for (int32_t i = node->parametersNumber() - 1; i > -1; --i) {
    var_map_.AddVar(node->parameterName(i));
    switch (node->parameterType(i)) {
      case VT_DOUBLE: {
        bcode_->add(BC_STOREDVAR); 
        bcode_->addInt16(var_map_.GetVarId(node->parameterName(i)));
        break;
      }
      case VT_INT: {
        bcode_->add(BC_STOREIVAR); 
        bcode_->addInt16(var_map_.GetVarId(node->parameterName(i)));
        break;
      }
      case VT_STRING: {
        bcode_->add(BC_STORESVAR); 
        bcode_->addInt16(var_map_.GetVarId(node->parameterName(i)));
        break;
      }
      default: {}
    }
  }
  node->body()->visit(this);
  bcode_ = temp;
  var_map_.DeleteScope();
}

void BytecodeVisitor::visitReturnNode(ReturnNode* node) {
  if (node->returnExpr()) {
    node->returnExpr()->visit(this);
  }
  bcode_->add(BC_RETURN);
}

void BytecodeVisitor::visitCallNode(CallNode* node) {
  uint16_t id = ((MvmCode *)code_)->_functionids[node->name()];
  node->visitChildren(this);
  bcode_->add(BC_CALL);
  bcode_->addInt16(id);
}

void BytecodeVisitor::visitNativeCallNode(mathvm::NativeCallNode*) {
  //stub
}

}
