#include <mathvm.h>
#include <parser.h>
#include <dlfcn.h>
#include "my_translator.h"
#include "my_interpreter.h"
#include "instruction_util.h"

using namespace mathvm;

static bool isCastPossible(VarType from, VarType to) {
  if (from == to) {
    return true;
  }

  return (from == VT_INT && to == VT_DOUBLE) ||
         (from == VT_DOUBLE && to == VT_INT) ||
         (from == VT_STRING && to == VT_INT) ||
         (from == VT_STRING && to == VT_DOUBLE);
}

Translator* Translator::create(const std::string& impl) {
  // just create a new bytecode translator
  return new BytecodeTranslatorImpl{};
}

MathVmTranslator::MathVmTranslator(Code* code)
    : _code(code), _typeOfTopOfStack(VT_VOID) {
}

Status* BytecodeTranslatorImpl::translate(const std::string& program, Code** code) {
  Parser parser;
  Status* parsingStatus = parser.parseProgram(program);

  if (parsingStatus->isError()) {
    return parsingStatus;
  }

  if (code == nullptr) {
    throw std::runtime_error("code argumed for translate function must be not null");
  }

  if (*code == nullptr) {
    *code = new InterpreterCodeImpl{};
  }


  MathVmTranslator translator{*code};

  Status* translationStatus = translator.run(parser.top());
  return translationStatus;
}

Status* MathVmTranslator::run(AstFunction* root) {
  BytecodeFunction* bytecodeFunction = new BytecodeFunction(root);
  _code->addFunction(bytecodeFunction);
  try {
    handleFunction(root);
  } catch (TranslationException const& e) {
    return Status::Error(e.what(), e.position());
  } catch (std::exception const& e) {
    return Status::Error(e.what());
  }

  return Status::Ok();
}

void MathVmTranslator::visitBinaryOpNode(BinaryOpNode* node) {
  TokenKind kind = node->kind();
  switch (getBinaryOperationType(kind)) {
    case ARITHMETIC:
      handleArithmeticOperation(node);
      break;
    case LOGICAL:
      handleLogicalOperation(node);
      break;
    case BITWISE:
      handleBitwiseOperation(node);
      break;
    case COMPARISON:
      handleComparisonOperation(node);
      break;
    case UNKNOWN:
      throw std::logic_error(std::string("Unknown operator found in birany operation: ") + tokenOp(kind));
  }
}

void MathVmTranslator::visitUnaryOpNode(UnaryOpNode* node) {
  visitNodeWithResult(node->operand());
  VarType type = _typeOfTopOfStack;
  if (type != VT_INT && type != VT_DOUBLE) {
    throw std::logic_error(std::string("Unexpected unary operand type: ") + typeToName(type));
  }
  TokenKind kind = node->kind();

  Bytecode* bytecode = getBytecode();
  switch (kind) {
    case tSUB:
      if (type == VT_DOUBLE) {
        bytecode->addInsn(BC_DNEG);
      } else {
        convertTopOfStackTo(VT_INT);
        bytecode->addInsn(BC_INEG);
      }
      break;
    case tNOT: {
      convertTopOfStackTo(VT_INT);

      Label setTrue(bytecode);
      bytecode->addBranch(BC_IFICMPE, setTrue);
      bytecode->addInsn(BC_POP);
      bytecode->addInsn(BC_ILOAD0);

      Label exit(bytecode);
      bytecode->addBranch(BC_JA, exit);

      bytecode->bind(setTrue);
      bytecode->addInsn(BC_POP);
      bytecode->addInsn(BC_ILOAD1);
      bytecode->bind(exit);
      break;
    }
    default:
      throw std::logic_error(std::string("Unexpected unary operation: ") + tokenOp(kind));
  }
}

void MathVmTranslator::visitStringLiteralNode(StringLiteralNode* node) {
  std::string const& literal = node->literal();
  Bytecode* bytecode = getBytecode();
  if (literal.empty()) {
    bytecode->addInsn(BC_SLOAD0);
    _typeOfTopOfStack = VT_STRING;
    return;
  }
  uint16_t literalId = _code->makeStringConstant(literal);
  bytecode->addInsn(BC_SLOAD);
  bytecode->addInt16(literalId);

  _typeOfTopOfStack = VT_STRING;
}

void MathVmTranslator::visitDoubleLiteralNode(DoubleLiteralNode* node) {
  double value = node->literal();
  Bytecode* bytecode = getBytecode();
  if (value == 0.) {
    bytecode->addInsn(BC_DLOAD0);
  } else if (value == 1.) {
    bytecode->addInsn(BC_DLOAD1);
  } else if (value == -1.) {
    bytecode->addInsn(BC_DLOADM1);
  } else {
    bytecode->addInsn(BC_DLOAD);
    bytecode->addDouble(value);
  }

  _typeOfTopOfStack = VT_DOUBLE;
}

void MathVmTranslator::visitIntLiteralNode(IntLiteralNode* node) {
  int64_t value = node->literal();
  Bytecode* bytecode = getBytecode();
  if (value == 0) {
    bytecode->addInsn(BC_ILOAD0);
  } else if (value == 1) {
    bytecode->addInsn(BC_ILOAD1);
  } else if (value == -1) {
    bytecode->addInsn(BC_ILOADM1);
  } else {
    bytecode->addInsn(BC_ILOAD);
    bytecode->addInt64(value);
  }

  _typeOfTopOfStack = VT_DOUBLE;
}

void MathVmTranslator::visitLoadNode(LoadNode* node) {
  AstVar const* variable = node->var();
  loadVariableToStack(variable);
}

void MathVmTranslator::visitStoreNode(StoreNode* node) {
  AstVar const* variable = node->var();

  node->value()->visit(this);

  Bytecode* bytecode = getBytecode();
  TokenKind operation = node->op();
  convertTopOfStackTo(variable->type());
  Instruction instruction;
  switch (operation) {
    case tASSIGN:
      storeTopOfStackToVariable(variable);
      break;
    case tINCRSET:
      loadVariableToStack(variable);

      instruction = getSumInstruction(variable->type());
      translationAssert(instruction != BC_INVALID,
                        "bad operand type for increment. Must be integer of double", node->position());
      bytecode->addInsn(instruction);

      storeTopOfStackToVariable(variable);
      break;
    case tDECRSET:
      loadVariableToStack(variable);
      swap();

      instruction = getSubInstruction(variable->type());
      translationAssert(instruction != BC_INVALID,
                        "bad operand type for decrement. Must be integer of double", node->position());
      bytecode->addInsn(instruction);

      storeTopOfStackToVariable(variable);
      break;
    default:
      throw std::logic_error(std::string("unexpected store operation: ") + tokenOp(operation));
  }
}

void MathVmTranslator::visitForNode(ForNode* node) {
  AstVar const* loopVariable = node->var();
  if (loopVariable->type() != VT_INT) {
    translationAssert(false, "for loop variable must be integer, but " + loopVariable->name() +
                             " is " + typeToName(loopVariable->type()), node->position());
  }

  AstNode* inExpr = node->inExpr();
  translationAssert(inExpr->isBinaryOpNode() && inExpr->asBinaryOpNode()->kind() == tRANGE,
                    "expression in for loop allows only binary range operation", inExpr->position());

  BinaryOpNode* rangeNode = inExpr->asBinaryOpNode();
  visitNodeWithResult(rangeNode->left());
  translationAssert(_typeOfTopOfStack == VT_DOUBLE || _typeOfTopOfStack == VT_INT,
                    "left bound in for expression must presents int or double value", rangeNode->left()->position());

  storeTopOfStackToVariable(loopVariable);
  visitNodeWithResult(rangeNode->right());
  translationAssert(_typeOfTopOfStack == VT_DOUBLE || _typeOfTopOfStack == VT_INT,
                    "right bound in for expression must presents int or double value", rangeNode->right()->position());

  convertTopOfStackTo(VT_INT);

  Bytecode* bytecode = getBytecode();
  Label startingLabel = Label(bytecode);
  Label endingLabel = Label(bytecode);

  bytecode->bind(startingLabel);
  loadVariableToStack(loopVariable);

  bytecode->addBranch(BC_IFICMPL, endingLabel);

  node->body()->visit(this);

  bytecode->addInsn(BC_ILOAD1);

  bytecode->addBranch(BC_JA, startingLabel);
  bytecode->bind(endingLabel);
}

void MathVmTranslator::visitWhileNode(WhileNode* node) {
  Bytecode* bytecode = getBytecode();
  Label beginningLabel = Label(bytecode);
  Label endingLabel = Label(bytecode);

  bytecode->bind(beginningLabel);
  node->whileExpr()->visit(this);
  convertTopOfStackTo(VT_INT);
  bytecode->addInsn(BC_ILOAD0);
  bytecode->addBranch(BC_IFICMPE, endingLabel);
  node->loopBlock()->visit(this);
  bytecode->addBranch(BC_JA, beginningLabel);
  bytecode->bind(endingLabel);
}

void MathVmTranslator::visitIfNode(IfNode* node) {
  visitNodeWithResult(node->ifExpr());
  convertTopOfStackTo(VT_INT);

  Bytecode* bytecode = getBytecode();
  Label elseLabel = Label(bytecode);
  Label exit = Label(bytecode);

  bytecode->addInsn(BC_ILOAD0);
  _typeOfTopOfStack = VT_INT;
  bytecode->addBranch(BC_IFICMPE, elseLabel);
  node->thenBlock()->visit(this);
  bytecode->addBranch(BC_JA, exit);

  bytecode->bind(elseLabel);
  BlockNode* elseBlock = node->elseBlock();
  if (elseBlock) {
    elseBlock->visit(this);
  }

  bytecode->bind(exit);
}

void MathVmTranslator::visitBlockNode(BlockNode* node) {
  Scope* scope = node->scope();
  Scope::VarIterator iter_var(scope);
  while (iter_var.hasNext()) {
    _context->addVariable(iter_var.next());
  }

  Scope::FunctionIterator definitionFunctionIterator(node->scope());
  while (definitionFunctionIterator.hasNext()) {
    AstFunction* astFunction = definitionFunctionIterator.next();
    BytecodeFunction* byte_func = (BytecodeFunction*) _code->functionByName(astFunction->name());
    if (!byte_func) {
      byte_func = new BytecodeFunction(astFunction);
      _code->addFunction(byte_func);

    } else {
      translationAssert(false, std::string("function ") + astFunction->name() + " already defined", node->position());
    }
  }

  Scope::FunctionIterator handleFunctionIterator(node->scope());
  while (handleFunctionIterator.hasNext()) {
    handleFunction(handleFunctionIterator.next());
  }

  for (uint32_t i = 0; i < node->nodes(); ++i) {
    node->nodeAt(i)->visit(this);
  }
}

void MathVmTranslator::visitFunctionNode(FunctionNode* node) {
  node->body()->visit(this);
}

void MathVmTranslator::visitReturnNode(ReturnNode* node) {
  VarType returnType = currentContext()->getCurrentFunction()->returnType();
  if (node->returnExpr()) {
    node->returnExpr()->visit(this);
    convertTopOfStackTo(returnType);
  }
  getBytecode()->addInsn(BC_RETURN);

  if (returnType != VT_VOID) {
    _typeOfTopOfStack = returnType;
  }
}

void MathVmTranslator::visitCallNode(CallNode* node) {
  std::string const& name = node->name();
  BytecodeFunction* function = (BytecodeFunction*) _code->functionByName(name);

  if (function->parametersNumber() != node->parametersNumber()) {
    translationAssert(false, std::string("function ") + name + " defined with" +
                             std::to_string(function->parametersNumber()) + " but called with " +
                             std::to_string(node->parametersNumber()), node->position());
  }

  if (function->parametersNumber() > 0) {
    for (uint32_t i = function->parametersNumber() - 1u; i != 0; --i) {
      visitNodeWithResult(node->parameterAt(i));
      convertTopOfStackTo(function->parameterType(i));
    }
  }

  Bytecode* bytecode = getBytecode();

  bytecode->addInsn(BC_CALL);
  bytecode->addInt16(function->id());

  VarType returnType = function->returnType();
  if (returnType != VT_VOID) {
    _typeOfTopOfStack = returnType;
  }
}

void MathVmTranslator::visitNativeCallNode(NativeCallNode* node) {
  void* nativeCode = dlsym(RTLD_DEFAULT, node->nativeName().c_str());
  if (!nativeCode) {
    throw std::logic_error("Native function not found");
  }

  uint16_t func_id = _code->makeNativeFunction(node->nativeName(), node->nativeSignature(), nativeCode);

  Bytecode* bytecode = getBytecode();
  bytecode->addInsn(BC_CALLNATIVE);
  bytecode->addUInt16(func_id);

  VarType returnType = node->nativeSignature()[0].first;
  if (returnType != VT_VOID) {
    _typeOfTopOfStack = returnType;
  }
}

void MathVmTranslator::visitPrintNode(PrintNode* node) {
  for (uint32_t i = 0; i < node->operands(); ++i) {
    node->operandAt(i)->visit(this);
    switch (_typeOfTopOfStack) {
      case VT_INT:
        getBytecode()->addInsn(BC_IPRINT);
        break;
      case VT_DOUBLE:
        getBytecode()->addInsn(BC_DPRINT);
        break;
      case VT_STRING:
        getBytecode()->addInsn(BC_SPRINT);
        break;
      default:
        throw std::logic_error("Wrong parameter of print method");
    }

    _typeOfTopOfStack = VT_VOID;
  }
}

void MathVmTranslator::loadVariableToStack(AstVar const* variable) {
  VarInfo info = currentContext()->getVarInfo(variable->name());
  VarType type = variable->type();

  uint16_t currentContextId = _context->getId();
  bool fromOuterScope = currentContextId != info.contextId;
  Instruction instruction = fromOuterScope
                            ? getLoadVarInstructionOuterScope(type)
                            : getLoadVariableInstructionLocalScope(type, info.localId);
  Bytecode* bytecode = getBytecode();
  bytecode->addInsn(instruction);
  if (fromOuterScope) {
    bytecode->addInt16(info.contextId);
    bytecode->addInt16(info.localId);
  } else if (info.localId >= 4) {
    bytecode->addInt16(info.localId);
  }

  _typeOfTopOfStack = type;
}

void MathVmTranslator::storeTopOfStackToVariable(AstVar const* variable) {
  VarInfo info = currentContext()->getVarInfo(variable->name());
  VarType type = variable->type();

  uint16_t currentContextId = _context->getId();
  bool fromLocalScope = currentContextId == info.contextId;
  Instruction instruction = fromLocalScope
                            ? getStoreVariableInstructionLocalScope(type, info.localId)
                            : getStoreVariableInstructionOuterScope(type);

  Bytecode* bytecode = getBytecode();
  bytecode->addInsn(instruction);
  if (!fromLocalScope) {
    bytecode->addInt16(info.contextId);
    bytecode->addInt16(info.localId);
  } else if (info.localId > 3) {
    bytecode->addInt16(info.localId);
  }

  _typeOfTopOfStack = VT_VOID;
}

void MathVmTranslator::handleArithmeticOperation(BinaryOpNode* node) {
  TokenKind kind = node->kind();

  visitNodeWithResult(node->left());
  node->left()->visit(this);

  VarType leftType = _typeOfTopOfStack;

  visitNodeWithResult(node->right());

  VarType rightType = _typeOfTopOfStack;

  VarType commonType = getBinaryOperationResultType(leftType, rightType);
  if (commonType == VT_INVALID) {
    throw std::logic_error(std::string("wrong binary operand types: ") + typeToName(leftType) +
                           tokenStr(kind) + typeToName(rightType));
  }

  Bytecode* bytecode = getBytecode();

  Instruction instruction = getArithmeticBinaryInstruction(commonType, kind);

  translationAssert(instruction != BC_INVALID,
                    std::string("unsupported arithmetical operation: ") + typeToName(leftType) +
                    " " + tokenStr(kind) + " " + typeToName(rightType), node->position());

  bytecode->addInsn(instruction);

  _typeOfTopOfStack = commonType;
}

void MathVmTranslator::handleLogicalOperation(BinaryOpNode* node) {
  TokenKind kind = node->kind();

  Bytecode* bytecode = getBytecode();
  Label exit(bytecode);
  if (kind == tAND) {
    Label setFalse(bytecode);
    bytecode->addInsn(BC_ILOAD0);
    visitNodeWithResult(node->left());
    convertTopOfStackTo(VT_INT);
    bytecode->addBranch(BC_IFICMPE, setFalse);
    bytecode->addInsn(BC_POP);

    visitNodeWithResult(node->right());
    convertTopOfStackTo(VT_INT);
    bytecode->addBranch(BC_IFICMPE, setFalse);
    bytecode->addInsn(BC_POP);
    bytecode->addInsn(BC_POP);
    bytecode->addInsn(BC_ILOAD1);
    bytecode->addBranch(BC_JA, exit);

    bytecode->bind(setFalse);
    bytecode->addInsn(BC_POP);
    bytecode->bind(exit);
    _typeOfTopOfStack = VT_INT;
    return;
  }

  if (kind == tOR) {
    Label setTrue(bytecode);
    bytecode->addInsn(BC_ILOAD0);

    visitNodeWithResult(node->left());
    convertTopOfStackTo(VT_INT);
    bytecode->addBranch(BC_IFICMPNE, setTrue);
    bytecode->addInsn(BC_POP);

    visitNodeWithResult(node->right());
    convertTopOfStackTo(VT_INT);
    bytecode->addBranch(BC_IFICMPNE, setTrue);
    bytecode->addInsn(BC_POP);
    bytecode->addBranch(BC_JA, exit);

    bytecode->bind(setTrue);
    bytecode->addInsn(BC_POP);
    bytecode->addInsn(BC_POP);
    bytecode->addInsn(BC_ILOAD1);

    bytecode->bind(exit);
    _typeOfTopOfStack = VT_INT;
    return;
  }

  translationAssert(false, std::string("unsupported logical operation: ") + " " + tokenStr(kind), node->position());
}

void MathVmTranslator::handleBitwiseOperation(BinaryOpNode* node) {
  visitNodeWithResult(node->left());
  convertTopOfStackTo(VT_INT);
  visitNodeWithResult(node->right());
  convertTopOfStackTo(VT_INT);

  TokenKind kind = node->kind();
  Instruction instruction = getBitwiseBinaryInstruction(kind);
  translationAssert(false, std::string("unsupported bitwise operation: ") + " " + tokenStr(kind), node->position());

  getBytecode()->addInsn(instruction);
}

void MathVmTranslator::handleComparisonOperation(BinaryOpNode* node) {
  TokenKind kind = node->kind();
  visitNodeWithResult(node->left());
  convertTopOfStackTo(VT_INT);

  visitNodeWithResult(node->right());
  convertTopOfStackTo(VT_INT);

  Instruction instruction = getComparisonBinaryInstruction(kind);

  Bytecode* bytecode = getBytecode();
  Label trueLabel(bytecode);
  Label exit(bytecode);

  bytecode->addBranch(instruction, trueLabel);
  bytecode->addInsn(BC_ILOAD0);
  bytecode->addBranch(BC_JA, exit);

  bytecode->bind(trueLabel);
  bytecode->addInsn(BC_ILOAD1);
  bytecode->bind(exit);

  bytecode->addInsn(BC_POP);
  bytecode->addInsn(BC_POP);
}

void MathVmTranslator::handleFunction(AstFunction* astFunction) {
  std::string const& name = astFunction->name();
  Scope* scope = astFunction->scope();
  BytecodeFunction* fun = (BytecodeFunction*) _code->functionByName(name);
  Context* innerContext = new Context(fun, scope, currentContext());
  _context = innerContext;

  for (uint32_t i = 0; i < astFunction->parametersNumber(); ++i) {
    std::string const& variableName = astFunction->parameterName(i);
    AstVar const* variable = astFunction->scope()->lookupVariable(variableName, false);
    storeTopOfStackToVariable(variable);
  }

  astFunction->node()->visit(this);

  fun->setScopeId(currentContext()->getId());
  fun->setLocalsNumber(currentContext()->getLocalsCount());

  _context = _context->getParent();
  delete innerContext;
}


void MathVmTranslator::convertTopOfStackTo(VarType to) {

  VarType stackType = _typeOfTopOfStack;
  if (stackType == to) {
    return;
  }


  translationAssert(isCastPossible(stackType, to),
                    std::string("Cast is not possible: ") + typeToName(stackType) + " -> " + typeToName(to),
                    Status::INVALID_POSITION);

  Bytecode* bytecode = getBytecode();
  if (stackType == VT_INT && to == VT_DOUBLE) {
    bytecode->addInsn(BC_I2D);
    _typeOfTopOfStack = VT_DOUBLE;
    return;
  }

  if (stackType == VT_DOUBLE && to == VT_INT) {
    bytecode->addInsn(BC_D2I);
    _typeOfTopOfStack = VT_INT;
    return;
  }

  if (stackType == VT_STRING && to == VT_INT) {
    bytecode->addInsn(BC_S2I);
    _typeOfTopOfStack = VT_INT;
  }

  if (stackType == VT_STRING && to == VT_DOUBLE) {
    convertTopOfStackTo(VT_INT);
    convertTopOfStackTo(VT_DOUBLE);
    return;
  }
}

// Possible casts:
// forall x: x != void : (x, x) -> x
// (int, double) -> double
// (int string) -> int
VarType MathVmTranslator::getBinaryOperationResultType(VarType first, VarType second) {
  if (first == second && first != VT_VOID) {
    return first;
  }

  switch (first) {
    case VT_DOUBLE:
      if (second == VT_INT) {
        return VT_DOUBLE;
      }
      break;
    case VT_INT:
      if (second == VT_DOUBLE) {
        return VT_DOUBLE;
      }
      if (second == VT_STRING) {
        return VT_INT;
      }
      break;
    case VT_STRING:
      if (second == VT_INT) {
        return VT_INT;
      }
    default:
      break;
  }

  return VT_INVALID;
}

void MathVmTranslator::visitNodeWithResult(AstNode* node) {
  node->visit(this);
  translationAssert(_typeOfTopOfStack != VT_VOID && _typeOfTopOfStack != VT_INVALID,
                    "Evaluation with result must put exacly one value to the stack", node->position());
}

void MathVmTranslator::swap() {
  getBytecode()->addInsn(BC_SWAP);
  _typeOfTopOfStack = VT_VOID;
}
