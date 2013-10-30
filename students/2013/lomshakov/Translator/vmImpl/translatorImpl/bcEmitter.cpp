//
// Created by Vadim Lomshakov on 10/26/13.
// Copyright (c) 2013 spbau.
//


#include "bcEmitter.h"

namespace mathvm {


  BytecodeEmitter::BytecodeEmitter(): _code(0), _currentAstFunctionScope(0) {}

  BytecodeEmitter::~BytecodeEmitter() {}

  void BytecodeEmitter::visitPrintNode(PrintNode *printNode) {
    AstNode* op = 0;
    for (uint32_t i = 0; i < printNode->operands(); i++) {
      op = printNode->operandAt(i);
      op->visit(this);

      insnSet.print(guessTypeExpr(op));
    }
  }

  void BytecodeEmitter::visitLoadNode(LoadNode *loadNode) {
    AstVar const *var = loadNode->var();
    LocationVar local = lookupCtxAndIdForVar(var->name());

    if (local.first == currentScopeId())
      insnSet.loadVar(var->type(), local.second);
    else
      insnSet.loadCtxVar(var->type(), local.first, local.second);
  }

  void BytecodeEmitter::visitForNode(ForNode *forNode) {
    assert(forNode->inExpr()->isBinaryOpNode());
    AstNode* lExpr = forNode->inExpr()->asBinaryOpNode()->left();
    AstNode* rExpr = forNode->inExpr()->asBinaryOpNode()->right();
    AstVar const* varLoop = forNode->var();

    LocationVar local = lookupCtxAndIdForVar(varLoop->name());
    // TODO bad...
    lExpr->visit(this);
    insnSet.prepareForVar(varLoop->type(), local.second, local.first == currentScopeId() ? 0 : &local.first);

    forNode->body()->visit(this);

    insnSet.prepareForCondition();
    rExpr->visit(this);

    insnSet.buildFor();
  }

  void BytecodeEmitter::visitIfNode(IfNode *ifNode) {
    ifNode->visit(this);

    insnSet.prepareThenBranch();
    ifNode->thenBlock();

    if (ifNode->elseBlock()) {
      insnSet.prepareElseBranch();
      ifNode->elseBlock()->visit(this);
    }

    insnSet.buildIf();
  }

  void BytecodeEmitter::visitWhileNode(WhileNode *whileNode) {
    insnSet.prepareWhileBlock();
    whileNode->loopBlock()->visit(this);
    insnSet.prepareWhileCondition();
    whileNode->whileExpr()->visit(this);
    insnSet.buildWhile();
  }

  void BytecodeEmitter::visitBinaryOpNode(BinaryOpNode *node) {

    AstBaseVisitor::visitBinaryOpNode(node);

    VarType type = guessTypeExpr(node);
    switch (node->kind()) {
      case tADD:
        insnSet.add(type);
        break;
      case tSUB:
        insnSet.sub(type);
        break;
      case tMUL:
        insnSet.mul(type);
        break;
      case tDIV:
        insnSet.div(type);
        break;
      case tMOD:
        insnSet.mod(type);
        break;
      case tAOR:
        insnSet.aor(type);
        break;
      case tAAND:
        insnSet.aand(type);
        break;
      case tAXOR:
        insnSet.axor(type);
        break;
      case tEQ:
        insnSet.eq(type);
        break;
      case tNEQ:
        insnSet.neq(type);
        break;
      case tLT:
        insnSet.lt(type);
        break;
      case tGT:
        insnSet.gt(type);
        break;
      case tGE:
        insnSet.ge(type);
        break;
      case tLE:
        insnSet.le(type);
        break;
      case tAND:
        // TODO
        break;
      case tOR:
        // TODO
        break;
      default:
        throw new logic_error("wrong operator kind " + string(tokenStr(node->kind())));
    }

  }

  void BytecodeEmitter::visitUnaryOpNode(UnaryOpNode *node) {

    AstBaseVisitor::visitUnaryOpNode(node);

    VarType type = guessTypeExpr(node);
    switch (node->kind()) {
      case tSUB:
        insnSet.neg(type);
        break;
      case tNOT:
        insnSet.bnot();
        break;
      case tADD:
        break;
      default:
        throw new logic_error("wrong operator kind " + string(tokenStr(node->kind())));
    }

  }

  void BytecodeEmitter::visitNativeCallNode(NativeCallNode *node) {
    //TODO
  }

  void BytecodeEmitter::visitCallNode(CallNode *node) {
    // push args on TOS
    AstBaseVisitor::visitCallNode(node);

    insnSet.call(getFunctionIdByName(node->name()));
  }

  void BytecodeEmitter::visitDoubleLiteralNode(DoubleLiteralNode *node) {
    insnSet.load(node->literal());
  }

  void BytecodeEmitter::visitStoreNode(StoreNode *storeNode) {
    AstVar const *var = storeNode->var();
    BytecodeEmitter::LocationVar local = lookupCtxAndIdForVar(var->name());

    // push value of rhs on TOS
    AstBaseVisitor::visitStoreNode(storeNode);

    if (storeNode->op() == tINCRSET) {
      var->type() == VT_INT ? insnSet.load((int64_t)1) : insnSet.load(1.0);
      insnSet.add(var->type());
    } else if (storeNode->op() == tDECRSET) {
      var->type() == VT_INT ? insnSet.load((int64_t)-1) : insnSet.load(-1.0);
      insnSet.add(var->type());
    }

    if (local.first == currentScopeId())
      insnSet.storeVar(var->type(), local.second);
    else
      insnSet.storeCtxVar(var->type(), local.first, local.second);
  }

  void BytecodeEmitter::visitStringLiteralNode(StringLiteralNode *node) {
    if (node->literal().empty())
      insnSet.sload0();
    else
      insnSet.sload(_code->makeStringConstant(node->literal()));
  }

  void BytecodeEmitter::visitIntLiteralNode(IntLiteralNode *node) {
    insnSet.load(node->literal());
  }

  void BytecodeEmitter::visitBlockNode(BlockNode *node) {
    Scope* blockScope = node->scope();

    for (Scope::FunctionIterator i(blockScope); i.hasNext(); ) {
      visitAstFunction(i.next());
    }
    makeMappingBlockLocals(blockScope);

    AstBaseVisitor::visitBlockNode(node);
  }

  void BytecodeEmitter::visitReturnNode(ReturnNode *node) {
    insnSet.rëturn();
  }

  void BytecodeEmitter::visitFunctionNode(FunctionNode *node) {
    AstBaseVisitor::visitBlockNode(node->body());
  }

  void BytecodeEmitter::visitAstFunction(AstFunction  *function) {
    pushAstFunction(function);
    makeMappingFunctionParametersAndLocalsById();

    for (Scope::FunctionIterator i(_currentAstFunctionScope); i.hasNext(); ) {
      visitAstFunction(i.next());
    }

    function->node()->visit(this);

    popAstFunction();
  }

  void BytecodeEmitter::emitCode(AstFunction* topLevel, Code *code) {
    _code = code;
    visitAstFunction(topLevel);
  }


//--------------------------------------------------------------------------------------------------------------------//
//-------------------------------------------- utility ---------------------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------//



  void BytecodeEmitter::makeMappingFunctionParametersAndLocalsById() {
    uint16_t id;

    // push parameters
    for (id = 0; id != currentBcFunction()->parametersNumber(); ++id) {
      _localsById.insert(make_pair(make_pair(currentBcFunction()->parameterName(id), currentScopeId()), id));
    }

    // push locals
    AstVar* v;
    for (Scope::VarIterator i(_currentAstFunctionScope); i.next(); ++id) {
      v = i.next();
      _localsById.insert(make_pair(make_pair(v->name(), currentScopeId()), id));
    }

    currentBcFunction()->setLocalsNumber((uint16_t) _currentAstFunctionScope->variablesCount());
  }

  void BytecodeEmitter::makeMappingBlockLocals(Scope *scope) {
    uint16_t id = (uint16_t) currentBcFunction()->localsNumber();
    AstVar* v;

    for (Scope::VarIterator i(scope); i.hasNext(); ) {
      v = i.next();
      if (_localsById.insert(make_pair(make_pair(v->name(), currentScopeId()), id)).second)
        ++id;
    }
    currentBcFunction()->setLocalsNumber(id);
  }

  void BytecodeEmitter::pushAstFunction(AstFunction *astFunction) {
    uint16_t idFoo;
    TranslatedFunction* foo;

    // lazy creates function metadata
    if ((foo = _code->functionByName(astFunction->name())) == 0) {
      foo = new BytecodeFunction(astFunction);
      idFoo = _code->addFunction(foo);
    } else {
      idFoo = foo->id();
    }

    _functionId.push_front(idFoo);
    _currentAstFunctionScope = astFunction->node()->body()->scope();
    insnSet.setContext(currentBcFunction()->bytecode());
  }

  void BytecodeEmitter::popAstFunction() {
    assert(!_functionId.empty());
    _functionId.pop_front();
    _currentAstFunctionScope = _currentAstFunctionScope->parent();
    insnSet.setContext(currentBcFunction()->bytecode());
  }

  BytecodeEmitter::LocationVar BytecodeEmitter::lookupCtxAndIdForVar(const string &name) {

    for (std::deque<uint16_t>::iterator i = _functionId.begin(); i != _functionId.end(); ++i) {
      if (isLocalVar(name, *i))
        return make_pair(*i, _localsById[make_pair(name, *i)]);
    }

    // impossible
    assert(false);
    return make_pair( -1, -1);
  }

  uint16_t BytecodeEmitter::getFunctionIdByName(string const &name) {
    AstFunction* astFoo;
    TranslatedFunction* foo;
    uint16_t idFoo;
    if ((astFoo = currentAstScope()->lookupFunction(name, true)) == 0) {
      throw std::logic_error("Call function " + name + " which is undeclared into " + currentBcFunction()->name());
    }

    // lazy gets id function
    if ((foo = _code->functionByName(name)) == 0) {
      foo = new BytecodeFunction(astFoo);
      idFoo = _code->addFunction(foo);
    } else {
      idFoo = foo->id();
    }

    return idFoo;
  }

  VarType BytecodeEmitter::guessTypeExpr(AstNode *node) {
    VarType type = VT_INVALID;
    while (node->isBinaryOpNode())
      node = node->asBinaryOpNode()->left();

    while (node->isUnaryOpNode())
      node = node->asUnaryOpNode()->operand();

    if (node->isLoadNode())
      type = node->asLoadNode()->var()->type();

    if (node->isIntLiteralNode())
      type = VT_INT;
    if (node->isDoubleLiteralNode())
      type = VT_DOUBLE;
    if (node->asStringLiteralNode())
      type = VT_STRING;

    if (node->isCallNode())
      type = currentAstScope()->lookupFunction(node->asCallNode()->name(), true)->returnType();

    if (node->isNativeCallNode())
      type = node->asNativeCallNode()->nativeSignature()[0].first;
    return type;
  }

  BytecodeEmitter &BytecodeEmitter::getInstance() {
    static BytecodeEmitter emitter;
    return emitter;
  }
}