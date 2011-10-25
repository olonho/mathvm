#include "TypeVisitor.h"

TypeVisitor::TypeVisitor(mathvm::AstFunction* top);
  topAstFunc = top;
}

void TypeVisitor::visit() { 
  mathvm::Scope* scope = new mathvm::Scope(0);
  scope->declareFunction(topAstFunc->node());
  topAstFunc->node()->visit(this);
}

void TypeVisitor::visitBinaryOpNode(mathvm::BinaryOpNode* node) {    
  using namespace mathvm;

  node->right()->visit(this);
  node->left()->visit(this);
  
  VarType resType = binNodeType(node->kind(), node->left(), node->right());
  nodeInfo.setNodeInfo(node, resType);
}

void TypeVisitor::visitUnaryOpNode(mathvm::UnaryOpNode* node) {
  using namespace mathvm;
  VarType resType;
  node->operand()->visit(this);
  NodeInfo& nop = nodeInfo.getNodeInfo(node->operand());
  if (nop.type == VT_INT && node->kind() == tNOT) {
    resType = VT_INT;        
  } else if (nop.type == VT_INT && node->kind() == tSUB) {
    resType = VT_INT;
  } else if (nop.type == VT_DOUBLE && node->kind() == tSUB) {
    resType = VT_DOUBLE;
  } else {
    typeError("Unary operation: " + std::string(tokenOp(node->kind())));
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
      typeError("Loading variable " + node->var()->name() + " which type is invalid");
  }
}

void TypeVisitor::visitStoreNode(mathvm::StoreNode* node) {
  using namespace mathvm;    
  nodeInfo.setNodeInfo(node, VT_VOID);

  node->value()->visit(this);    
  NodeInfo& val = nodeInfo.getNodeInfo(node->value());
  VarInfo& var  = varInfo.topSymbolData(node->var()->name());

  VarType uppedType = upType(val.type, var.type);
  if (resType != val.type) {
    val.convertTo = resType;
  }
  if (resType != var.type) {
    typeError("Cannot cast var " + node->var()->name() +  " to upper type");
  }

  switch(node->op()) {
    case tASSIGN:
      if (var.type != VT_INT &&
          var.type != VT_DOUBLE && 
          var.type != VT_STRING) {
        typeError("Assigning to invalid type");
      }
      break;
    case tINCRSET:
      if (var.type != VT_INT &&
          var.type != VT_DOUBLE) {
        typeError("+= to invalid type");
      }
      break;
    case tDECRSET:
      if (var.type != VT_INT &&
          var.type != VT_DOUBLE)
        typeError("-= to invalid type");
      }
      break;
    default:
      typeError("only =, +=, -= are permitted");
  }
}

void TypeVisitor::visitForNode(mathvm::ForNode* node) {
  using namespace mathvm;    
  BinaryOpNode* op = dynamic_cast<BinaryOpNode*>(node->inExpr());
  if (!op)
    typeError("for node needs binary operation to evaluate");
  if (op->kind() != tRANGE)
    typeError("for node needs range .. operation to evaluate");
  
  nodeInfo.setNodeInfo(node, VT_VOID);
  VarInfo& var = varInfo.topSymbolData(node->var()->name());
  node->inExpr()->visit(this);
  node->body()->visit(this);
  NodeInfo& nl = varInfo.getNodeInfo(op->left());
  NodeInfo& nr = varInfo.getNodeInfo(op->right());
  if ()
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
  mathvm::Scope::VarIterator it(node->scope());
  //push
  while(it.hasNext()) {
    mathvm::AstVar* curr = it.next();
    varInfo.pushSymbolData(curr->name(), VarInfo(curr->name(), curr->type()));
  }    
  mathvm::Scope::FunctionIterator fit(node->scope());
  while(fit.hasNext()) { 
    AstFunction func(fit.next()->node(), 0);
    Params params;
    params.returnType = func.returnType();
    for(uint32_t i = 0; i < func.parametersNumber(); ++i) {
      ParamInfo par;
      par.type = func.parameterType(i);
      par.index = i;
      par.name = func.parameterName(i);
      params.setParamInfo(par);
    }
    funcParams.pushSymbolData(func->name(), params);
  }
  //visit
  fit = mathvm::Scope::FunctionIterator(node->scope());
  while(fit.hasNext()) { 
    fit.next()->node()->visit(this);
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
    funcParams.popSymbolData(fit->node()->next()->name());
  }
}

void TypeVisitor::visitCallNode(mathvm::CallNode* node) {
  using namespace mathvm;
  Params& params = funcParams.topSymbolData(node->name());   
  setNodeInfo(node, params.returnType);
  
  if (node->parametersNumber() != params.size()) {
    typeError("passing invalid number of parameter to function " + f->name());
  }

  for (uint32_t i = 0; i < node->parametersNumber(); i++) {
    node->parameterAt(i)->visit(this);
    NodeInfo& nparValue = nodeInfo.getNodeInfo(node->parameterAt(i));
    if (params.getParamInfo(i).type != nparValue.type) {
      mathvm::VarType t = upType(params.getParamInfo(i).type, nparValue.type);
      if (t != nparValue.type)
        nparValue.convertTo = t;
    }
    switch(npar.byte) {
      case VT_INT: case VT_DOUBLE: case VT_STRING:
        break;
      default:
        typeError("passing parameter with uknown type to function " + f->name());
    }
  }
}

void TypeVisitor::visitFunctionNode(mathvm::FunctionNode* node) {
  using namespace mathvm;

  AstFunction func(node, 0);
  for(uint32_t i = 0; i < func.parametersNumber(); ++i) {
    varInfo.pushSymbolData(VarInfo(func.parameterName(i), func.parameterType(i)));
  }
  node->body()->visit(this);
  for(uint32_t i = 0; i < func.parametersNumber(); ++i) {
    varInfo.popSymbolData(func.parameterName(i));
  }
}

void TypeVisitor::visitReturnNode(mathvm::ReturnNode* node) {
  using namespace mathvm;
  nodeInfo.setNodeInfo(node, VT_VOID);
  node->returnExpr()->visit(this);
}

void TypeVisitor::visitPrintNode(mathvm::PrintNode* node) {
  using namespace mathvm;
  setNodeInfo(node, VT_VOID);

  for (uint32_t i = 0; i < node->operands(); ++i) {
    node->operandAt(i)->visit(this);
    NodeInfo& nop = nodeInfo.getNodeInfo(node->operandAt(i));
    switch (nop.type) {
      case VT_INT:
      case VT_DOUBLE:
      case VT_STRING:
        break;
      default:
        typeError("Print: unprintable type");
        break;
    }
  }
}

void TypeVisitor::typeError(std::string str) { 
  cCode().addByte(mathvm::BC_INVALID);
  cCode().dump(std::cerr);
  throw TranslationException("Type check error: " + str + "\n");
  //std::cerr << "Error during translation (" << str << ")" << std::endl;   
  //exit(-1); 
}

void  TypeVisitor::binNodeType(mathvm::TokenKind op, mathvm::AstNode* left, mathvm::AstNode* right) {
  using namespace mathvm;
  NodeInfo& nl = nodeInfo.getNodeInfo(left);
  NodeInfo& nr = nodeInfo.getNodeInfo(right);
  if (nl.type == VT_INT && nt.type == VT_INT) {
    switch (op) {
      case tADD:case tSUB:case tMUL:case tDIV:case tOR:case tAND:
      case tEQ:case tNEQ:case tGT:case tLT:case tGE:case tLE:case tRANGE:
        return VT_INT;
      break;
      default:
        typeError(std::string("operation ") + tokenOp(op) + " on int and int is not permitted");
    }
  } else {
    bool ok = false;
    if (nl.type == VT_INT && nr.type == VT_DOUBLE) {
      nl.convertTo = VT_DOUBLE; ok = true;
    } else if (nr.type == VT_INT && nl.type == VT_DOUBLE) {
      nr.convertTo = VT_DOUBLE; ok = true;
    }
    if (ok) {
      switch (op) {
        case tADD:case tSUB:case tMUL:case tDIV:case tRANGE:
          return VT_DOUBLE;
        break;
        case tEQ:case tNEQ:case tGT:case tLT:case tGE:case tLE:
          return VT_INT;
        break;
        default:
          typeError(std::string("operation ") + tokenOp(op) + " on doubles is not permitted" );
      }
    } else typeError(std::string("binary operation") + tokenOp(op)  + "on unsupported types");
  }
}
