#include "TypeVisitor.h"

TypeVisitor::TypeVisitor(mathvm::AstFunction* top) {
  topAstFunc = top;
}

void TypeVisitor::visit() { 
  topAstFunc->node()->visit(this);
}

void TypeVisitor::visitBinaryOpNode(mathvm::BinaryOpNode* node) {    
  using namespace mathvm;

  node->right()->visit(this);
  node->left()->visit(this);
  
  VarType resType = binNodeType(node, node->left(), node->right());
  nodeInfo.setNodeInfo(node, resType);
}

void TypeVisitor::visitUnaryOpNode(mathvm::UnaryOpNode* node) {
  using namespace mathvm;
  VarType resType = VT_INVALID;
  node->operand()->visit(this);
  NodeInfo& nop = nodeInfo.getNodeInfo(node->operand());
  if (nop.type == VT_INT && node->kind() == tNOT) {
    resType = VT_INT;        
  } else if (nop.type == VT_INT && node->kind() == tSUB) {
    resType = VT_INT;
  } else if (nop.type == VT_DOUBLE && node->kind() == tSUB) {
    resType = VT_DOUBLE;
  } else {
    typeError("Unary operation: " + std::string(tokenOp(node->kind())), node);
  }    
  nodeInfo.setNodeInfo(node, resType);
}

void TypeVisitor::visitStringLiteralNode(mathvm::StringLiteralNode* node) {    
  nodeInfo.setNodeInfo(node, mathvm::VT_STRING);
}

void TypeVisitor::visitDoubleLiteralNode(mathvm::DoubleLiteralNode* node) {
  nodeInfo.setNodeInfo(node, mathvm::VT_DOUBLE);
}

void TypeVisitor::visitIntLiteralNode(mathvm::IntLiteralNode* node) {
  nodeInfo.setNodeInfo(node, mathvm::VT_INT);
}

void TypeVisitor::visitLoadNode(mathvm::LoadNode* node) {
  using namespace mathvm;    
  VarInfo& var = varInfo.topSymbolData(node->var()->name());
  nodeInfo.setNodeInfo(node, var.type);

  switch(var.type) {
    case VT_INT:
    case VT_DOUBLE:
    case VT_STRING:
      break;
    default:
      typeError("Loading variable " + node->var()->name() + " which type is invalid", node);
  }
}

void TypeVisitor::visitStoreNode(mathvm::StoreNode* node) {
  using namespace mathvm;    
  nodeInfo.setNodeInfo(node, VT_VOID);

  node->value()->visit(this);    
  NodeInfo& val = nodeInfo.getNodeInfo(node->value());
  VarInfo& var  = varInfo.topSymbolData(node->var()->name());

  VarType uppedType = upType(val.type, var.type);
  if (uppedType != val.type) {
    val.convertTo = uppedType;
  }
  if (uppedType != var.type) {
    typeError("Cannot cast var " + node->var()->name() +  " to upper type", node);
  }

  switch(node->op()) {
    case tASSIGN:
      if (var.type != VT_INT &&
          var.type != VT_DOUBLE && 
          var.type != VT_STRING) {
        typeError("Assigning to invalid type", node);
      }
      break;
    case tINCRSET:
      if (var.type != VT_INT &&
          var.type != VT_DOUBLE) {
        typeError("+= to invalid type", node);
      }
      break;
    case tDECRSET:
      if (var.type != VT_INT &&
          var.type != VT_DOUBLE) {
        typeError("-= to invalid type", node);
      }
      break;
    default:
      typeError("only =, +=, -= are permitted", node);
  }
}

void TypeVisitor::visitForNode(mathvm::ForNode* node) {
  using namespace mathvm;    
  BinaryOpNode* op = dynamic_cast<BinaryOpNode*>(node->inExpr());
  if (!op)
    typeError("for node needs binary operation to evaluate", node);
  if (op->kind() != tRANGE)
    typeError("for node needs range .. operation to evaluate", node);
  
  nodeInfo.setNodeInfo(node, VT_VOID);
  VarInfo& var = varInfo.topSymbolData(node->var()->name());
  node->inExpr()->visit(this);
  node->body()->visit(this);
  NodeInfo& nl = nodeInfo.getNodeInfo(op->left());
  NodeInfo& nr = nodeInfo.getNodeInfo(op->right());

  VarType uppedType = upType(var.type, nl.type);
  if (uppedType != nl.type) {
    nl.convertTo = uppedType;
  }
  uppedType = upType(var.type, nr.type);
  if (nr.type != uppedType) {
    nr.convertTo = uppedType;
  }
}

void TypeVisitor::visitWhileNode(mathvm::WhileNode* node) {
  using namespace mathvm;
  nodeInfo.setNodeInfo(node, VT_VOID);
  node->whileExpr()->visit(this);
  node->loopBlock()->visit(this);
}

void TypeVisitor::visitIfNode(mathvm::IfNode* node) {
  using namespace mathvm;    
  nodeInfo.setNodeInfo(node, VT_VOID);
  node->ifExpr()->visit(this);
  node->thenBlock()->visit(this);
  if (node->elseBlock()) {
    node->elseBlock()->visit(this);
  }
}

void TypeVisitor::visitBlockNode(mathvm::BlockNode* node) {
  using namespace mathvm;
  nodeInfo.setNodeInfo(node, VT_VOID);
  mathvm::Scope::VarIterator it(node->scope());
  //push
  while(it.hasNext()) {
    mathvm::AstVar* curr = it.next();
    varInfo.pushSymbolData(curr->name(), VarInfo(curr->name(), curr->type()));
  }    
  mathvm::Scope::FunctionIterator fit(node->scope());
  while(fit.hasNext()) { 
    AstFunction& func = *fit.next();
    Params params;
    params.returnType = func.returnType();
    for(uint32_t i = 0; i < func.parametersNumber(); ++i) {
      ParamInfo par;
      par.type = func.parameterType(i);
      par.index = i;
      par.name = func.parameterName(i);
      params.setParamInfo(par);
    }
    funcParams.pushSymbolData(func.name(), params);
  }
  //visit
  mathvm::Scope::FunctionIterator fit1(node->scope());
  while(fit1.hasNext()) { 
    fit1.next()->node()->visit(this);
  }
  node->visitChildren(this);
  //pop
  it = mathvm::Scope::VarIterator(node->scope());
  while(it.hasNext()) {
    mathvm::AstVar* curr = it.next();
    varInfo.popSymbolData(curr->name());
  }
  fit = mathvm::Scope::FunctionIterator(node->scope());
  while(fit.hasNext()) { 
    funcParams.popSymbolData(fit.next()->node()->name());
  }
}

void TypeVisitor::visitCallNode(mathvm::CallNode* node) {
  using namespace mathvm;
  Params& params = funcParams.topSymbolData(node->name());   
  nodeInfo.setNodeInfo(node, params.returnType);
  
  if (node->parametersNumber() != params.size()) {
    typeError("passing invalid number of parameter to function " + node->name(), node);
  }

  for (uint32_t i = 0; i < node->parametersNumber(); i++) {
    node->parameterAt(i)->visit(this);
    NodeInfo& npar = nodeInfo.getNodeInfo(node->parameterAt(i));
    if (params.getParamInfo(i).type != npar.type) {
      mathvm::VarType t = upType(params.getParamInfo(i).type, npar.type);
      if (t != npar.type)
        npar.convertTo = t;
    }
    switch(npar.type) {
      case VT_INT: case VT_DOUBLE: case VT_STRING:
        break;
      default:
        typeError("passing parameter with uknown type to function " + node->name(), node);
    }
  }
}

void TypeVisitor::visitFunctionNode(mathvm::FunctionNode* node) {
  using namespace mathvm;
  nodeInfo.setNodeInfo(node, VT_VOID);

  for(uint32_t i = 0; i < node->parametersNumber(); ++i) {
    varInfo.pushSymbolData(node->parameterName(i), VarInfo(node->parameterName(i), node->parameterType(i)));
  }
  const char* parentFuncName = curFuncName;
  curFuncName = node->name().c_str();
  node->body()->visit(this);
  curFuncName = parentFuncName;
  for(uint32_t i = 0; i < node->parametersNumber(); ++i) {
    varInfo.popSymbolData(node->parameterName(i));
  }
}

void TypeVisitor::visitReturnNode(mathvm::ReturnNode* node) {
  using namespace mathvm;
  nodeInfo.setNodeInfo(node, VT_VOID);
  if (node->returnExpr()) {
    node->returnExpr()->visit(this);
    NodeInfo& info = nodeInfo.getNodeInfo(node->returnExpr());
    VarType uppedType = upType(funcParams.topSymbolData(curFuncName).returnType, info.type);
    if (uppedType != info.type) {
      info.convertTo = uppedType;
    }
  }
}

void TypeVisitor::visitPrintNode(mathvm::PrintNode* node) {
  using namespace mathvm;
  nodeInfo.setNodeInfo(node, VT_VOID);

  for (uint32_t i = 0; i < node->operands(); ++i) {
    node->operandAt(i)->visit(this);
    NodeInfo& nop = nodeInfo.getNodeInfo(node->operandAt(i));
    switch (nop.type) {
      case VT_INT:
      case VT_DOUBLE:
      case VT_STRING:
        break;
      default:
        typeError("Print: unprintable type", node);
        break;
    }
  }
}

void TypeVisitor::typeError(std::string str, mathvm::AstNode* node) {
  throw new TranslationException("Type check error: " + str + "\n", node);
}

mathvm::VarType TypeVisitor::binNodeType(mathvm::BinaryOpNode* node, mathvm::AstNode* left, mathvm::AstNode* right) {
  using namespace mathvm;
  mathvm::TokenKind op = node->kind();
  NodeInfo& nl = nodeInfo.getNodeInfo(left);
  NodeInfo& nr = nodeInfo.getNodeInfo(right);
  if (nl.type == VT_INT && nr.type == VT_INT) {
    switch (op) {
      case tADD:case tSUB:case tMUL:case tDIV:case tOR:case tAND:
      case tEQ:case tNEQ:case tGT:case tLT:case tGE:case tLE:
        return VT_INT;
      break;
      case tRANGE:
        return VT_VOID;
      break;
      default:
        typeError(std::string("operation ") + tokenOp(op) + " on int and int is not permitted", node);
    }
  } else {
    bool ok = false;
    if (nl.type == VT_DOUBLE && nr.type == VT_DOUBLE) {
      ok = true;
    } else if (nl.type == VT_INT && nr.type == VT_DOUBLE) {
      nl.convertTo = VT_DOUBLE; ok = true;
    } else if (nr.type == VT_INT && nl.type == VT_DOUBLE) {
      nr.convertTo = VT_DOUBLE; ok = true;
    }
    if (ok) {
      switch (op) {
        case tADD:case tSUB:case tMUL:case tDIV:
          return VT_DOUBLE;
        break;
        case tEQ:case tNEQ:case tGT:case tLT:case tGE:case tLE:
          return VT_INT;
        break;
        case tRANGE:
          return VT_VOID;
        break;
        default:
          typeError(std::string("operation ") + tokenOp(op) + " on doubles is not permitted", node);
      }
    } else typeError(std::string("binary operation") + tokenOp(op)  + "on unsupported types", node);
  }
  return *((VarType*)0);
}

