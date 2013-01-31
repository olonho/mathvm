#include <string>
#include <map>
#include <sstream>
#include <fstream>

#include "parser.h"
#include "visitors.h"
#include "bytecode_gen.h"

using namespace mathvm;
using namespace std;

struct BCGenerationException : public exception {
   string s;
   BCGenerationException(string ss) : s(ss) {}
   const char* what() const throw() { 
    return s.c_str(); 
   }
   ~BCGenerationException() throw() {}
};

/** Main ast-traversing method that generates Code */
void BytecodeGenerator::generate(const char * source, Code * acode) {
  if (parser.parseProgram(source) != 0) {
    cout << "Parsing Error occurred!" << endl;
    return;
  }
  
  code = acode;
  Scope * topScope = parser.top()->scope();
  firstRun(topScope);
  secondRun(topScope);
#if defined(DEBUG) && defined(DEBUG_FILENAME)
  fstream filestr;
  filestr.open(DEBUG_FILENAME, fstream::out | fstream::app);
  code->disassemble(filestr);
  filestr.close();
#endif
}

void BytecodeGenerator::firstRun(Scope * scope) {
  Scope::FunctionIterator it(scope);
  while (it.hasNext()) {
    AstFunction *func = it.next();
    BytecodeFunction *bcf = new BytecodeFunction(func);
    currentFun = code->addFunction(bcf);
    initFunLocals(currentFun);
    Scope::VarIterator vi(func->scope());
    fBC = bcf->bytecode();    
    while (vi.hasNext()) {
    	AstVar * var = vi.next();
    	int index = localsCounter[currentFun];
			incFunLocals(currentFun);
			varIds[var] = make_pair(currentFun, index);
      storeVar(var);
    }
	}
	for (size_t i = 0; i < scope->childScopeNumber(); ++i) {
    firstRun(scope->childScopeAt(i));
  }
}

void BytecodeGenerator::secondRun(Scope * scope) {
  Scope::FunctionIterator it(scope);
  while (it.hasNext()) {
    AstFunction *func = it.next();
    BytecodeFunction *bcf =
        (BytecodeFunction*) code->functionByName(func->name());
    fBC = bcf->bytecode();
    currentFun = bcf->id();
    returnType = func->returnType();
    func->node()->body()->visit(this);
    bcf->setLocalsNumber(localsCounter[currentFun]);
  }

  for (size_t i = 0; i < scope->childScopeNumber(); ++i) {
    secondRun(scope->childScopeAt(i));
  }
}

void BytecodeGenerator::convert(const VarType& from, const VarType& to) {
  if (from == to) 
  	return;
  if (from == VT_DOUBLE && to == VT_INT) {
    fBC->addInsn(BC_D2I);
    TOSType = VT_INT;
  } else if (from == VT_INT && to == VT_DOUBLE) {
    fBC->addInsn(BC_I2D);
    TOSType = VT_DOUBLE;
  } else if (from == VT_STRING && to == VT_INT) {
    fBC->addInsn(BC_S2I);
    TOSType = VT_INT;
  } else {
  	throw BCGenerationException("Unsupported conversation");
  }
}

void BytecodeGenerator::loadCTXVar(const AstVar *var) {
  switch (var->type()) {
  case VT_INT:
    fBC->addInsn(BC_LOADCTXIVAR);
    TOSType = VT_INT;
    break;
  case VT_DOUBLE:
    fBC->addInsn(BC_LOADCTXDVAR);
    TOSType = VT_DOUBLE;
    break;
  case VT_STRING:
    fBC->addInsn(BC_LOADCTXSVAR);
    TOSType = VT_STRING;
    break;
  default:
  	throw BCGenerationException("unknown type loading");
    break;
  }
  fBC->addInt16(varIds[var].first);
  fBC->addInt16(varIds[var].second);
}

void BytecodeGenerator::storeCTXVar(const AstVar *var) {
  switch (var->type()) {
  case VT_INT:
    fBC->addInsn(BC_STORECTXIVAR);
    break;
  case VT_DOUBLE:
    fBC->addInsn(BC_STORECTXDVAR);
    break;
  case VT_STRING:
    fBC->addInsn(BC_STORECTXSVAR);
    break;
  default:
  	throw BCGenerationException("unknown type storing");
    break;
  }
  fBC->addInt16(varIds[var].first);
  fBC->addInt16(varIds[var].second);
}

void BytecodeGenerator::loadVar(const AstVar *var) {
  switch (var->type()) {
  case VT_INT:
    fBC->addInsn(BC_LOADIVAR);
    TOSType = VT_INT;
    break;
  case VT_DOUBLE:
    fBC->addInsn(BC_LOADDVAR);
    TOSType = VT_DOUBLE;
    break;
  case VT_STRING:
    fBC->addInsn(BC_LOADSVAR);
    TOSType = VT_STRING;
    break;	
  default:
  	throw BCGenerationException("unknown type loading");
    break;
  }
  fBC->addInt16(varIds[var].second);
}

void BytecodeGenerator::storeVar(const AstVar *var) {
  switch (var->type()) {
 	case VT_INT:
    fBC->addInsn(BC_STOREIVAR);
    break;
  case VT_DOUBLE:
    fBC->addInsn(BC_STOREDVAR);
    break;
  case VT_STRING:
    fBC->addInsn(BC_STORESVAR);
    break;
  default:
  	throw BCGenerationException("unknown type storing");
    break;
  }
  fBC->addInt16(varIds[var].second);
}

void BytecodeGenerator::visitBinaryOpNode(BinaryOpNode * node) {
  node->left()->visit(this);
  VarType left = TOSType;
  if (left == VT_STRING) {
  	fBC->addInsn(BC_S2I);
  	left = VT_INT;
  }
  node->right()->visit(this);
  VarType right = TOSType;
  if (right == VT_STRING) {
  	fBC->addInsn(BC_S2I);
  	right = VT_INT;
  }

  Label end(fBC);
  Label iload1(fBC);
  bool comparisson = false;

  if (left == VT_INT && right == VT_INT) {
    switch (node->kind()) {
    case tADD: fBC->addInsn(BC_IADD); break;
    case tMUL: fBC->addInsn(BC_IMUL); break;
    case tSUB: fBC->addInsn(BC_SWAP); fBC->addInsn(BC_ISUB); break;
    case tDIV: fBC->addInsn(BC_SWAP); fBC->addInsn(BC_IDIV); break;
    case tMOD: fBC->addInsn(BC_SWAP); fBC->addInsn(BC_IMOD); break;
    case tOR: 
      fBC->addInsn(BC_DUMP);
      fBC->addInsn(BC_SWAP);
      fBC->addInsn(BC_DUMP);
      fBC->addInsn(BC_IADD);
      fBC->addInsn(BC_DUMP);
      break;
    case tAND:
    	fBC->addInsn(BC_DUMP);
      fBC->addInsn(BC_SWAP);
      fBC->addInsn(BC_DUMP);
    	fBC->addInsn(BC_IMUL);
    	break;
    case tNEQ: fBC->addBranch(BC_IFICMPNE, iload1); comparisson = true; break;
    case tEQ: fBC->addBranch(BC_IFICMPE, iload1); comparisson = true; break;
    case tLE: fBC->addBranch(BC_IFICMPGE, iload1); comparisson = true; break;
    case tGE: fBC->addBranch(BC_IFICMPLE, iload1); comparisson = true; break;
    case tLT: fBC->addBranch(BC_IFICMPG, iload1); comparisson = true; break;
    case tGT: fBC->addBranch(BC_IFICMPL, iload1); comparisson = true; break;
    case tRANGE: /* Just do nothing with 2 VT_INT'S */ break;
    default:
      throw BCGenerationException("Unsupported integer operation");
    }
    if (comparisson) {
    	fBC->addInsn(BC_ILOAD0);
      fBC->addBranch(BC_JA, end);
      fBC->bind(iload1);
      fBC->addInsn(BC_ILOAD1);
      fBC->bind(end);
    }
    TOSType = VT_INT;
    return;
  }

  convert(right, VT_DOUBLE);
  fBC->addInsn(BC_SWAP);
  convert(left, VT_DOUBLE);

  switch (node->kind()) {
  case tADD: fBC->addInsn(BC_DADD); break;
  case tMUL: fBC->addInsn(BC_DMUL); break;
  case tSUB: fBC->addInsn(BC_DSUB); break;
  case tDIV: fBC->addInsn(BC_DDIV); break;
  default:
    throw BCGenerationException("Unsupported double operation");
  }
  TOSType = VT_DOUBLE;
}

void BytecodeGenerator::visitUnaryOpNode(UnaryOpNode * node) {
  node->operand()->visit(this);
  switch (node->kind()) {
  case tNOT:
    if (TOSType != VT_INT)
      throw BCGenerationException("Unsupported not with non integers");
    fBC->addInsn(BC_DUMP);
    fBC->addInsn(BC_ILOAD1);
    fBC->addInsn(BC_ISUB);
    break;
  case tSUB:
    switch (TOSType) {
    case VT_INT: fBC->addInsn(BC_INEG); break;
    case VT_DOUBLE: fBC->addInsn(BC_DNEG); break;
    case VT_STRING: 
    	fBC->addInsn(BC_S2I); 
    	fBC->addInsn(BC_INEG);
    	TOSType = VT_INT;
    	break;
    default:
      throw BCGenerationException("Unsupported unary operand type");
    }
    break;
  default:
    throw BCGenerationException("Unsupported unary operation");
  }
}

void BytecodeGenerator::visitStringLiteralNode(StringLiteralNode * node) {
  fBC->addInsn(BC_SLOAD);
  fBC->addInt16(code->makeStringConstant(node->literal()));
  TOSType = VT_STRING;
}

void BytecodeGenerator::visitDoubleLiteralNode(DoubleLiteralNode * node) {
  fBC->addInsn(BC_DLOAD);
  fBC->addDouble(node->literal());
  TOSType = VT_DOUBLE;
}

void BytecodeGenerator::visitIntLiteralNode(IntLiteralNode * node) {
  fBC->addInsn(BC_ILOAD);
  fBC->addInt64(node->literal());
  TOSType = VT_INT;
}

void BytecodeGenerator::visitLoadNode(LoadNode * node) {
  loadVar(node->var());
}

void BytecodeGenerator::visitStoreNode(StoreNode * node) {
  node->value()->visit(this);
  VarType type = node->var()->type();
  convert(TOSType, type);
  switch (node->op()) {
  case tINCRSET:
    loadVar(node->var());
    if (type == VT_INT)
      fBC->addInsn(BC_IADD);
    else
      fBC->addInsn(BC_DADD);
    storeVar(node->var());
    break;
  case tDECRSET:
    loadVar(node->var());
    if (type == VT_INT)
      fBC->addInsn(BC_ISUB);
    else
      fBC->addInsn(BC_DSUB);
    storeVar(node->var());
    break;
  case tASSIGN:
  	storeVar(node->var());
    break;
  default:
    throw BCGenerationException("Unsupported unary operation");
  }
  
 }

/*****************************************************************************/

void BytecodeGenerator::visitForNode(ForNode * node) {

}

/*****************************************************************************/

void BytecodeGenerator::visitWhileNode(WhileNode * node) {

}

/*****************************************************************************/

void BytecodeGenerator::visitIfNode(IfNode * node) {

}

/*****************************************************************************/

void BytecodeGenerator::visitBlockNode(BlockNode * node) {
  Scope::VarIterator iter(node->scope());

  while (iter.hasNext()) {
    int index = localsCounter[currentFun];
    incFunLocals(currentFun);
    varIds[iter.next()] = make_pair(currentFun, index);
  }

  node->visitChildren(this);
}

/*****************************************************************************/

void BytecodeGenerator::visitFunctionNode(FunctionNode * node) {

}

/*****************************************************************************/

void BytecodeGenerator::visitReturnNode(ReturnNode * node) {

}

/*****************************************************************************/

void BytecodeGenerator::visitCallNode(CallNode * node) {
  size_t n = node->parametersNumber();
  TranslatedFunction * fun = code->functionByName(node->name());
  for (size_t i = 0; i < n; ++i) {
    node->parameterAt(n + i - 1)->visit(this);
    convert(TOSType, fun->parameterType(n + i - 1));
  }
  fBC->addInsn(BC_CALL);
  fBC->addInt16(fun->id());
  TOSType = fun->returnType();
}

void BytecodeGenerator::visitNativeCallNode(NativeCallNode * node) {
	// Couldn't find native call syntax
}

void BytecodeGenerator::visitPrintNode(PrintNode * node) {
  for (size_t i = 0; i < node->operands(); i++) {
    AstNode *operand = node->operandAt(i);
    operand->visit(this);
    switch (TOSType) {
    case VT_INT: fBC->addInsn(BC_IPRINT); break;
    case VT_DOUBLE: fBC->addInsn(BC_DPRINT); break;
    case VT_STRING: fBC->addInsn(BC_SPRINT); break;
    default:
    	throw BCGenerationException("Unknown type");
    }
  }
}