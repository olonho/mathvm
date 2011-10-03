#include "bytecode_visitor.h"
#include <cstdio>
#include <cstdlib>

namespace mathvm {
  
void BytecodeVisitor::visitBinaryOpNode(BinaryOpNode* node) {
  // node->left()->visit(this);
  // printf(" %s ", tokenOp(node->kind()));
  // node->right()->visit(this);
}

void BytecodeVisitor::visitUnaryOpNode(UnaryOpNode* node) {
  // printf("%s(", tokenOp(node->kind()));
  // node->operand()->visit(this);
  // printf(")");
}

// **************************************** //
void BytecodeVisitor::visitStringLiteralNode(
    StringLiteralNode* node) {
  bcode_->add(BC_SLOAD);
  bcode_->addInt16(code_->makeStringConstant(node->literal()));
}

// **************************************** //
void BytecodeVisitor::visitDoubleLiteralNode(
    DoubleLiteralNode* node) {
  bcode_->add(BC_DLOAD);
  bcode_->addDouble(node->literal());
}

// ************************************* //
void BytecodeVisitor::visitIntLiteralNode(
    IntLiteralNode* node) {
  bcode_->add(BC_ILOAD);
  bcode_->addInt64(node->literal());
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
  bcode_->add(var_map_.GetVarId(node->var()->name()));
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
          bcode_->add(var_map_.GetVarId(node->var()->name()));
          bcode_->add(BC_DADD);
          break;
        }
        case tDECRSET: {
          bcode_->add(BC_LOADDVAR); 
          bcode_->add(var_map_.GetVarId(node->var()->name()));
          bcode_->add(BC_DSUB);
          break;
        }
      default: {}
      }      
      bcode_->add(BC_STOREDVAR); 
      bcode_->add(var_map_.GetVarId(node->var()->name()));
      break;
    }
    case VT_INT: { 
      node->value()->visit(this);
      switch (node->op()) {
        case tINCRSET: {
          bcode_->add(BC_LOADIVAR); 
          bcode_->add(var_map_.GetVarId(node->var()->name()));
          bcode_->add(BC_IADD);
          break;
        }
        case tDECRSET: {
          bcode_->add(BC_LOADIVAR); 
          bcode_->add(var_map_.GetVarId(node->var()->name()));
          bcode_->add(BC_ISUB);
          break;
        }
        default: {}
      }
      bcode_->add(BC_STOREIVAR); 
      bcode_->add(var_map_.GetVarId(node->var()->name()));
      break;
    }
    case VT_STRING: {
      node->value()->visit(this);
      bcode_->add(BC_STORESVAR); 
      bcode_->add(var_map_.GetVarId(node->var()->name()));
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
  bcode_->add(var_map_.GetVarId(node->var()->name()));
  Label forBegin(bcode_);
  bcode_->bind(forBegin);
  node->body()->visit(this);
  node->inExpr()->asBinaryOpNode()->right()->visit(this);
  bcode_->add(BC_LOADIVAR);
  bcode_->add(var_map_.GetVarId(node->var()->name()));
  bcode_->add(BC_ILOAD1);
  bcode_->add(BC_IADD);
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
  bcode_->addInsn(BC_ILOAD0);
  bcode_->addBranch(BC_IFICMPE, elseBranch); 
  node->thenBlock()->visit(this);  
  bcode_->addBranch(BC_JA, ifEnd); 
  bcode_->bind(elseBranch);
  node->elseBlock()->visit(this);
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
  }
  node->visitChildren(this);
  var_map_.DeleteScope();
}

void BytecodeVisitor::visitPrintNode(PrintNode* node) {
  // printf("print(");
  // for (uint32_t i = 0; i < node->operands() - 1; i++) {
  //   node->operandAt(i)->visit(this);
  //   printf(", ");
  // }
  // node->operandAt(node->operands() - 1)->visit(this);
  // printf(");\n");
}

void BytecodeVisitor::visitFunctionNode(FunctionNode* node) {
  // TBD
}

void BytecodeVisitor::visitReturnNode(ReturnNode* node) {
  // TBD
}

void BytecodeVisitor::visitCallNode(CallNode* node) {
  // TBD
}

}
