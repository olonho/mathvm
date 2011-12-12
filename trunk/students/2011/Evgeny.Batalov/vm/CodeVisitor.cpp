#include "CodeVisitor.h"

void* MyNativeFunction::libcHandler;

CodeVisitor::CodeVisitor(mathvm::AstFunction* top, 
                         const FunctionContexts& funcContexts, 
                         const FunctionNodeToIndex& funcNodeToIndex, 
                         const IndexToFunctionNode& indexToFuncNode,
                         const NodeInfos& nodeInfo) 
                         : topAstFunc(top)
                         , funcNodeToIndex(funcNodeToIndex)
                         , indexToFuncNode(indexToFuncNode)
                         , nodeInfo(nodeInfo)
                         , curBytecode(0)
                         , curBlock(0)
{
  executable = new Executable();
  FunctionContexts::const_iterator it = funcContexts.begin();
  /*for(; it != funcContexts.end(); ++it) {
    std::cout << "Function context: " << it->funcName << std::endl;
  }
  it = funcContexts.begin();*/

  for(; it != funcContexts.end(); ++it) {
    executable->getMetaData().push_back(TranslatableFunction(*it)); 
  }
  MyNativeFunction::initNatives();
}

void CodeVisitor::visit() { 
  using namespace mathvm;
  //start translation
  topAstFunc->node()->visit(this);
  //add memory for locals of top function
  MyBytecode *mainCode = executable->getMain()->bytecode();
  const FunctionContext& mainContext = executable->getMetaData()[0].getProto();
  mainCode->data().insert(mainCode->data().begin(), 
                          mainContext.locals.size() * 2,
                          BC_ILOAD0);
  for(size_t i = 0; i < mainContext.typeInfo.localsType.size(); ++i) {
    switch(mainContext.typeInfo.localsType[i]) {
      case VT_INT: case VT_STRING:
        //BC_ILOAD0 yet
        mainCode->data()[i*2] = BCA_VM_SPECIFIC;
        mainCode->data()[i*2 + 1] = BC_ILOAD0;
      break;
      case VT_DOUBLE:
        mainCode->data()[i*2] = BCA_VM_SPECIFIC;
        mainCode->data()[i*2 + 1] = BC_DLOAD0;
      break;
      default:
      break;
    }
  }
}

void CodeVisitor::visitBinaryOpNode(mathvm::BinaryOpNode* node) {    
  using namespace mathvm;

  if (node->kind() == tOR || node->kind() == tAND) {
    //Label lazyLabel(&cCode());
    node->left()->visit(this);
    //putLazyLogic(node->kind(), lazyLabel);
    node->right()->visit(this);
    //cCode().bind(lazyLabel);
    //nextBCJumped();
  } else {
    //TOS: left, right
    node->right()->visit(this);
    node->left()->visit(this);
  }
  NodeInfo& n = nodeInfo.getNodeInfo(node);
  NodeInfo& nr = nodeInfo.getNodeInfo(node->right());
  NodeInfo& nl = nodeInfo.getNodeInfo(node->left());
  procBinNode(node, n.type, nl.type, nr.type);
  cast(node);
}

void CodeVisitor::visitUnaryOpNode(mathvm::UnaryOpNode* node) {
  using namespace mathvm;
  node->operand()->visit(this);

  NodeInfo& info = nodeInfo.getNodeInfo(node);
  if (node->kind() == tNOT) {
    Label lblFalse(&cCode()); //addres where result of negation will be false
    Label lblTrue (&cCode()); //addres where result of negation will be true
    cCode().addByte(BC_ILOAD0);
    cCode().addBranch(BC_IFICMPNE, lblFalse);
    cCode().addByte(BCA_LOGICAL_OP_RES);
    cCode().addByte(BC_ILOAD1);
    cCode().addByte(BCA_LOGICAL_OP_RES_END);
    cCode().addBranch(BC_JA, lblTrue);
    cCode().bind(lblFalse);
    nextBCJumped();
    cCode().addByte(BCA_LOGICAL_OP_RES);
    cCode().addByte(BC_ILOAD0);
    cCode().addByte(BCA_LOGICAL_OP_RES_END);
    cCode().bind(lblTrue);
    nextBCJumped();
  } else if (node->kind() == tSUB) {
    if (info.type == VT_INT) {
      cCode().addByte(BC_INEG);
    }
    else if (info.type == VT_DOUBLE) {
      cCode().addByte(BC_DNEG);
    }
  }    
  cast(node);
}

void CodeVisitor::visitStringLiteralNode(mathvm::StringLiteralNode* node) {    
  using namespace mathvm;
  //store string literal in memory
  std::string s = node->literal();
  /*size_t pos = 0;
  while(std::string::npos != (pos = s.find('\n', pos))) {
    s.replace(pos, 1,  "\\n");
  }*/

  uint16_t newId = executable->makeStringConstant(s);
  cCode().addByte(BC_SLOAD);
  cCode().addUInt16(newId);
  cast(node);
}

void CodeVisitor::visitDoubleLiteralNode(mathvm::DoubleLiteralNode* node) {
  using namespace mathvm;
  cCode().addByte(BC_DLOAD);
  cCode().addDouble(node->literal());
  cast(node);
}

void CodeVisitor::visitIntLiteralNode(mathvm::IntLiteralNode* node) {
  using namespace mathvm;
  cCode().addByte(BC_ILOAD);
  cCode().addInt64(node->literal());
  cast(node);
}

void CodeVisitor::visitLoadNode(mathvm::LoadNode* node) {
  using namespace mathvm;    
  NodeInfo& info = nodeInfo.getNodeInfo(node);

  switch(info.type) {
    case VT_INT:
      cCode().addByte(BC_LOADIVAR);
      cCode().addUInt16(executable->getMetaData()[curFuncId].getAddress(node->var()->name()));
      break;
    case VT_DOUBLE:
      cCode().addByte(BC_LOADDVAR);
      cCode().addUInt16(executable->getMetaData()[curFuncId].getAddress(node->var()->name()));
      break;
    case VT_STRING:
      cCode().addByte(BC_LOADSVAR);
      cCode().addUInt16(executable->getMetaData()[curFuncId].getAddress(node->var()->name()));
      break;
    default:
      transError("No code generated for load var " + node->var()->name(), node);
  }
  cast(node);
}

void CodeVisitor::visitStoreNode(mathvm::StoreNode* node) {
  using namespace mathvm;    
  node->value()->visit(this);    
  NodeInfo& val = nodeInfo.getNodeInfo(node->value());
  VarType type = (val.convertTo == VT_INVALID ? val.type : val.convertTo);
  switch(node->op()) {
    case tASSIGN:
      if (type == VT_INT) {
        cCode().addByte(BC_STOREIVAR); 
        cCode().addUInt16(executable->getMetaData()[curFuncId].getAddress(node->var()->name()));
      }
      else if (type == VT_DOUBLE) {
        cCode().addByte(BC_STOREDVAR); 
        cCode().addUInt16(executable->getMetaData()[curFuncId].getAddress(node->var()->name()));
      }
      else if (type == VT_STRING) {
        cCode().addByte(BC_STORESVAR);
        cCode().addUInt16(executable->getMetaData()[curFuncId].getAddress(node->var()->name()));
      } else {
        transError("No code generated for store var " + node->var()->name(), node);
      }
      break;
    case tINCRSET:
      if (type == VT_INT) {
        cCode().addByte(BC_LOADIVAR); 
        cCode().addUInt16(executable->getMetaData()[curFuncId].getAddress(node->var()->name()));
        cCode().addByte(BC_IADD);
        cCode().addByte(BC_STOREIVAR);
        cCode().addUInt16(executable->getMetaData()[curFuncId].getAddress(node->var()->name()));
      }
      else if (type == VT_DOUBLE) {
        cCode().addByte(BC_LOADDVAR); 
        cCode().addUInt16(executable->getMetaData()[curFuncId].getAddress(node->var()->name()));
        cCode().addByte(BC_DADD);
        cCode().addByte(BC_STOREDVAR);
        cCode().addUInt16(executable->getMetaData()[curFuncId].getAddress(node->var()->name()));
      }
      else {
        transError("No code generated for store var " + node->var()->name(), node);
      }
      break;
    case tDECRSET:
      if (type == VT_INT) {
        cCode().addByte(BC_LOADIVAR); 
        cCode().addUInt16(executable->getMetaData()[curFuncId].getAddress(node->var()->name()));
        cCode().addByte(BC_ISUB);
        cCode().addByte(BC_STOREIVAR);
        cCode().addUInt16(executable->getMetaData()[curFuncId].getAddress(node->var()->name()));
      }
      else if (type == VT_DOUBLE) {
        cCode().addByte(BC_LOADDVAR); 
        cCode().addUInt16(executable->getMetaData()[curFuncId].getAddress(node->var()->name()));
        cCode().addByte(BC_DSUB);
        cCode().addByte(BC_STOREDVAR);
        cCode().addUInt16(executable->getMetaData()[curFuncId].getAddress(node->var()->name()));
      }
      else {
        transError("No code generated for store var " + node->var()->name(), node);
      }
      break;
    default:
      transError("No code generated for store var " + node->var()->name(), node);
  }
  cast(node);
}

void CodeVisitor::visitForNode(mathvm::ForNode* node) {
  using namespace mathvm;    

  BinaryOpNode* op = dynamic_cast<BinaryOpNode*>(node->inExpr());
  Label lblLoopCheck(&cCode());
  Label lblEnd(&cCode());

  //NodeInfo& opn = nodeInfo.getNodeInfo(op);
  NodeInfo& ln = nodeInfo.getNodeInfo(op->left());
  NodeInfo& rn = nodeInfo.getNodeInfo(op->left());
  VarType rangeType = upType(ln.type, rn.type);
 
  if (rangeType == VT_INT) {
    op->left()->visit(this);
    cCode().addByte(BC_STOREIVAR);
    cCode().addUInt16(executable->getMetaData()[curFuncId].getAddress(node->var()->name()));
    cCode().bind(lblLoopCheck);
    nextBCJumped();
    op->right()->visit(this);
    cCode().addByte(BC_LOADIVAR);
    cCode().addUInt16(executable->getMetaData()[curFuncId].getAddress(node->var()->name()));
    cCode().addBranch(BC_IFICMPG, lblEnd); 
  } else
  if (rangeType == VT_DOUBLE) {
    op->left()->visit(this);
    cCode().addByte(BC_STOREDVAR);
    cCode().addUInt16(executable->getMetaData()[curFuncId].getAddress(node->var()->name()));
    cCode().bind(lblLoopCheck);
    nextBCJumped();
    op->right()->visit(this);
    cCode().addByte(BC_LOADDVAR);
    cCode().addUInt16(executable->getMetaData()[curFuncId].getAddress(node->var()->name()));
    cCode().addByte(BC_DCMP);
    cCode().addByte(BC_ILOAD0);
    cCode().addBranch(BC_IFICMPL, lblEnd);
  }

  node->body()->visit(this);

  if (rangeType == VT_INT) {
    cCode().addByte(BC_LOADIVAR);
    cCode().addUInt16(executable->getMetaData()[curFuncId].getAddress(node->var()->name()));
    cCode().addByte(BC_ILOAD1);
    cCode().addByte(BC_IADD);
    cCode().addByte(BC_STOREIVAR);
    cCode().addUInt16(executable->getMetaData()[curFuncId].getAddress(node->var()->name()));
    cCode().addBranch(BC_JA, lblLoopCheck);
  } else
  if (rangeType == VT_DOUBLE) {
    cCode().addByte(BC_LOADDVAR);
    cCode().addUInt16(executable->getMetaData()[curFuncId].getAddress(node->var()->name()));
    cCode().addByte(BC_DLOAD1);
    cCode().addByte(BC_DADD);
    cCode().addByte(BC_STOREDVAR);
    cCode().addUInt16(executable->getMetaData()[curFuncId].getAddress(node->var()->name()));
    cCode().addBranch(BC_JA, lblLoopCheck);
  }

  cCode().bind(lblEnd);
  nextBCJumped();
  cast(node);
}

void CodeVisitor::visitWhileNode(mathvm::WhileNode* node) {
  using namespace mathvm;

  Label lblWhile(&cCode());
  Label lblEnd(&cCode());

  cCode().bind(lblWhile); {
    nextBCJumped();
    node->whileExpr()->visit(this);
    cCode().addByte(BC_ILOAD0);
    cCode().addBranch(BC_IFICMPE, lblEnd);
    node->loopBlock()->visit(this);
    cCode().addBranch(BC_JA, lblWhile);
  } cCode().bind(lblEnd);
  nextBCJumped();
  cast(node);
}

void CodeVisitor::visitIfNode(mathvm::IfNode* node) {
  using namespace mathvm;    
  Label lblElse(&cCode());
  Label lblEnd(&cCode());

  node->ifExpr()->visit(this);
  cCode().addByte(BC_ILOAD0);
  cCode().addBranch(BC_IFICMPE, lblElse);
  node->thenBlock()->visit(this);
  cCode().addBranch(BC_JA, lblEnd);
  cCode().bind(lblElse);
  nextBCJumped();
  if (node->elseBlock()) {
    node->elseBlock()->visit(this);
  }
  cCode().bind(lblEnd);
  nextBCJumped();
  cast(node);
}

void CodeVisitor::visitBlockNode(mathvm::BlockNode* node) {
  using namespace mathvm;
  //BlockNode* parentBlock = &cBlock();
  //curBlock = node;
  mathvm::Scope::VarIterator it(node->scope());

  Strings scopeSymbols; 
  while(it.hasNext()) {
    mathvm::AstVar* curr = it.next();
    scopeSymbols.push_back(curr->name());
  }   
  executable->getMetaData()[curFuncId].pushSymbols(scopeSymbols);

  mathvm::Scope::FunctionIterator fit(node->scope());
  while(fit.hasNext()) {
    FunctionNode* func = fit.next()->node();
    funcId.pushSymbolData(func->name(), funcNodeToIndex[func]);
  }
  fit = mathvm::Scope::FunctionIterator(node->scope());
  while(fit.hasNext()) {
    fit.next()->node()->visit(this);
  }
  for(uint32_t i = 0; i < node->nodes(); ++i) {
    node->nodeAt(i)->visit(this);
    if (node->nodeAt(i)->isCallNode()) {
      CallNode* cn = dynamic_cast<CallNode*>(node->nodeAt(i));
      NodeInfo& info = nodeInfo.getNodeInfo(cn);
      if (info.type != VT_VOID) {
        cCode().addByte(BC_POP);
      }
    }
  }

  fit = mathvm::Scope::FunctionIterator(node->scope());
  while(fit.hasNext()) {
    funcId.popSymbolData(fit.next()->node()->name());
  }
  executable->getMetaData()[curFuncId].popSymbols(scopeSymbols);
  //curBlock = parentBlock;
  cast(node);
}

void CodeVisitor::visitCallNode(mathvm::CallNode* node) {
  using namespace mathvm;
  NodeInfo& info = nodeInfo.getNodeInfo(node);
  TranslatableFunction& calledFunc = executable->getMetaData()[funcId.topSymbolData(node->name())];
  TranslatableFunction& callingFunc = executable->getMetaData()[curFuncId];
  calledFunc.genCallingCode(callingFunc, this, cCode(), node, info.type);
  cast(node);
}

void CodeVisitor::visitNativeCallNode(mathvm::NativeCallNode* node) {
}

void CodeVisitor::visitFunctionNode(mathvm::FunctionNode* node) {
  using namespace mathvm;
  
  TranslatableFunction& transFunc =
    executable->getMetaData()[funcNodeToIndex[node]];

  if (transFunc.getProto().type == FT_NATIVE) {
    executable->addNativeFunc(funcNodeToIndex[node],
        new MyNativeFunction(node->name(), 
          node->returnType(), node->signature()));
  } else {
    AstFunction *astFunc = new AstFunction(node, 0);
    MyBytecodeFunction *func = 
      new MyBytecodeFunction(astFunc, &transFunc);

    executable->addFunc(funcNodeToIndex[node], func);

    MyBytecode* parentBytecode = &cCode();
    curBytecode = func->bytecode();
    size_t parentFuncId = curFuncId;
    curFuncId = funcNodeToIndex[node];

    node->body()->visit(this);
    cCode().addByte(mathvm::BC_STOP);

    curBytecode = parentBytecode;
    curFuncId = parentFuncId;
  }
  cast(node);
}

void CodeVisitor::visitReturnNode(mathvm::ReturnNode* node) {
  using namespace mathvm;
  if (node->returnExpr()) {
    node->returnExpr()->visit(this);
  }
  cast(node);
  cCode().addByte(BC_RETURN);
}

void CodeVisitor::visitPrintNode(mathvm::PrintNode* node) {
  using namespace mathvm;
  for (uint32_t i = 0; i < node->operands(); ++i) {
    node->operandAt(i)->visit(this);
    NodeInfo& nop = nodeInfo.getNodeInfo(node->operandAt(i));
    switch (nop.type) {
      case VT_INT:
        cCode().addByte(BC_IPRINT);
        break;
      case VT_DOUBLE:
        cCode().addByte(BC_DPRINT);
        break;
      case VT_STRING:
        cCode().addByte(BC_SPRINT);
        break;
      default:
        transError("Print: couldn't generate code", node);
        break;
    }
  }
  cast(node);
}

void CodeVisitor::transError(std::string str, mathvm::AstNode* node) { 
  cCode().addByte(mathvm::BC_INVALID);
  cCode().dump(std::cerr);
  throw new TranslationException("Error during translation: " + str + "\n", node);
  //DEBUG("Error during translation (" << str << ")" << std::endl);   
  //exit(-1); 
}

void CodeVisitor::putLazyLogic(mathvm::TokenKind op, mathvm::Label& lbl) {
  using namespace mathvm;

  Label lbl1(&cCode());
  //type checking will be done by procBinNode
  if (op == tAND) {
    cCode().addByte(BC_ILOAD0);
    cCode().addBranch(BC_IFICMPNE, lbl1);
    cCode().addByte(BCA_LOGICAL_OP_RES);
    cCode().addByte(BC_ILOAD0);
    cCode().addByte(BCA_LOGICAL_OP_RES_END);
    cCode().addBranch(BC_JA, lbl);
    cCode().bind(lbl1);
    nextBCJumped();

  } else if (op == tOR) {
    cCode().addByte(BC_ILOAD0);
    cCode().addBranch(BC_IFICMPE, lbl1);
    cCode().addByte(BCA_LOGICAL_OP_RES);
    cCode().addByte(BC_ILOAD1);
    cCode().addByte(BCA_LOGICAL_OP_RES_END);
    cCode().addBranch(BC_JA, lbl);
    cCode().bind(lbl1);
    nextBCJumped();
  }
}

void CodeVisitor::cast(mathvm::AstNode* node) {
  using namespace mathvm;
  NodeInfo& info = nodeInfo.getNodeInfo(node);
  if (info.type == VT_INT && info.convertTo == VT_DOUBLE) {
    cCode().addByte(BC_I2D);
    info.type = VT_DOUBLE;
  }
  if (info.type == VT_DOUBLE && info.convertTo == VT_INT) {
    cCode().addByte(BC_D2I);
    info.type = VT_INT;
  }
}

void  CodeVisitor::procBinNode(mathvm::BinaryOpNode* node, mathvm::VarType resType, 
                               mathvm::VarType lNodeType, mathvm::VarType rNodeType) {
  using namespace mathvm;
  
  Label lblTrue(&cCode()); //result of op is true
  Label lblFalse(&cCode());//result of op is false    
  TokenKind op = node->kind();
  if (lNodeType == VT_INT && rNodeType == VT_INT) {
    switch (op) {
      case tADD:
        cCode().addByte(BC_IADD);
        break;
      case tSUB:
        cCode().addByte(BC_ISUB);
        break;
      case tMUL:
        cCode().addByte(BC_IMUL);
        break;
      case tDIV:
        cCode().addByte(BC_IDIV);
        break;
      case tMOD:
        cCode().addByte(BC_IMOD);
        break;
      case tOR:{
        Label lblTrue1(&cCode()); //result of op is true but POP
        cCode().addByte(BC_ILOAD0);
        cCode().addBranch(BC_IFICMPNE, lblTrue1);
        cCode().addByte(BC_ILOAD0);
        cCode().addBranch(BC_IFICMPNE, lblTrue);
        cCode().addByte(BCA_LOGICAL_OP_RES);
        cCode().addByte(BC_ILOAD0);
        cCode().addByte(BCA_LOGICAL_OP_RES_END);
        cCode().addBranch(BC_JA, lblFalse);
        cCode().bind(lblTrue1);
        nextBCJumped();
        cCode().addByte(BCA_VM_SPECIFIC);
        cCode().addByte(BC_POP);
        cCode().bind(lblTrue);
        nextBCJumped();
        cCode().addByte(BCA_LOGICAL_OP_RES);
        cCode().addByte(BC_ILOAD1);
        cCode().addByte(BCA_LOGICAL_OP_RES_END);
        cCode().bind(lblFalse);
        nextBCJumped();}
        //lazy logic has checked first operand and it was false
        /*cCode().addByte(BC_ILOAD0);
        cCode().addBranch(BC_IFICMPNE, lblTrue);
        cCode().addByte(BCA_LOGICAL_OP_RES);
        cCode().addByte(BC_ILOAD0);
        cCode().addByte(BCA_LOGICAL_OP_RES_END);
        cCode().addBranch(BC_JA, lblFalse);
        cCode().bind(lblTrue);
        nextBCJumped();
        cCode().addByte(BCA_LOGICAL_OP_RES);
        cCode().addByte(BC_ILOAD1);
        cCode().addByte(BCA_LOGICAL_OP_RES_END);
        cCode().bind(lblFalse);
        nextBCJumped();*/
        break;
      case tAND:{
        Label lblFalse1(&cCode()); //result of op is false but POP
        cCode().addByte(BC_ILOAD0);
        cCode().addBranch(BC_IFICMPE, lblFalse1);
        cCode().addByte(BC_ILOAD0);
        cCode().addBranch(BC_IFICMPE, lblFalse);
        cCode().addByte(BCA_LOGICAL_OP_RES);
        cCode().addByte(BC_ILOAD1);
        cCode().addByte(BCA_LOGICAL_OP_RES_END);
        cCode().addBranch(BC_JA, lblTrue);

        cCode().bind(lblFalse1);
        nextBCJumped();
        cCode().addByte(BCA_VM_SPECIFIC);
        cCode().addByte(BC_POP);
        cCode().bind(lblFalse);
        nextBCJumped();
        cCode().addByte(BCA_LOGICAL_OP_RES);
        cCode().addByte(BC_ILOAD0);
        cCode().addByte(BCA_LOGICAL_OP_RES_END);
        cCode().bind(lblTrue);}
        //lazy logic has checked first operand and it was true
        /*cCode().addByte(BC_ILOAD0);
        cCode().addBranch(BC_IFICMPE, lblFalse);
        cCode().addByte(BCA_LOGICAL_OP_RES);
        cCode().addByte(BC_ILOAD1);
        cCode().addByte(BCA_LOGICAL_OP_RES_END);
        cCode().addBranch(BC_JA, lblTrue);
        cCode().bind(lblFalse);
        nextBCJumped();
        cCode().addByte(BCA_LOGICAL_OP_RES);
        cCode().addByte(BC_ILOAD0);
        cCode().addByte(BCA_LOGICAL_OP_RES_END);
        cCode().bind(lblTrue);*/
        nextBCJumped();
        break;
      case tEQ:case tNEQ:case tGT:case tLT:case tGE:case tLE:
        if (op == tEQ)
          cCode().addBranch(BC_IFICMPE, lblTrue);
        else if (op == tNEQ)
          cCode().addBranch(BC_IFICMPNE, lblTrue);
        else if (op == tGT)
          cCode().addBranch(BC_IFICMPG, lblTrue);
        else if (op == tLT)
          cCode().addBranch(BC_IFICMPL, lblTrue);
        else if (op == tGE)
          cCode().addBranch(BC_IFICMPGE, lblTrue);
        else if (op == tLE)
          cCode().addBranch(BC_IFICMPLE, lblTrue);
        cCode().addByte(BCA_LOGICAL_OP_RES);
        cCode().addByte(BC_ILOAD0);
        cCode().addByte(BCA_LOGICAL_OP_RES_END);
        cCode().addBranch(BC_JA, lblFalse);
        cCode().bind(lblTrue);
        nextBCJumped();
        cCode().addByte(BCA_LOGICAL_OP_RES);
        cCode().addByte(BC_ILOAD1);
        cCode().addByte(BCA_LOGICAL_OP_RES_END);
        cCode().bind(lblFalse);
        nextBCJumped();
        break;
      case tRANGE:
        //left on TOS and then right yet
        break;
      default:
        transError(std::string("operation ") + tokenOp(op) + " on int and int is not permitted");
    }
  } else
    switch (op) {
      case tADD:
        cCode().addByte(BC_DADD);
        break;
      case tSUB:
        cCode().addByte(BC_DSUB);
        break;
      case tMUL:
        cCode().addByte(BC_DMUL);
        break;
      case tDIV:
        cCode().addByte(BC_DDIV);
        break;
      case tEQ:case tNEQ:case tGT:case tLT:case tGE:case tLE:
        //DCMP returns 1 if left > right and -1 if left < right
        cCode().addByte(BC_DCMP);
        cCode().addByte(BC_ILOAD0);
        if (op == tEQ)
          cCode().addBranch(BC_IFICMPE, lblTrue);
        else if (op == tNEQ)
          cCode().addBranch(BC_IFICMPNE, lblTrue);
        else if (op == tGT)
          cCode().addBranch(BC_IFICMPL, lblTrue);
        else if (op == tLT)
          cCode().addBranch(BC_IFICMPG, lblTrue);
        else if (op == tGE)
          cCode().addBranch(BC_IFICMPLE, lblTrue);
        else if (op == tLE)
          cCode().addBranch(BC_IFICMPGE, lblTrue);

        cCode().addByte(BCA_LOGICAL_OP_RES);
        cCode().addByte(BC_ILOAD0);
        cCode().addByte(BCA_LOGICAL_OP_RES_END);
        cCode().addBranch(BC_JA, lblFalse);
        cCode().bind(lblTrue);
        nextBCJumped();
        cCode().addByte(BCA_LOGICAL_OP_RES);
        cCode().addByte(BC_ILOAD1);
        cCode().addByte(BCA_LOGICAL_OP_RES_END);
        cCode().bind(lblFalse); 
        nextBCJumped();
        break;
      default:
        transError(std::string("operation ") + tokenOp(op) + " on int and double is not permitted", node);
    }
}

void CodeVisitor::nextBCJumped() {
  DEBUG("Remembering jump label: " << cCode().data().size() << std::endl);
  size_t addition = 0; 
  if (curFuncId == 0) { 
    addition = executable->getMetaData()[0].getProto().locals.size() * 2;
  }
  executable->getMetaData()[curFuncId].bcAddressJumped(cCode().current() + addition);
}
