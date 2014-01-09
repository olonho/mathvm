//
// Created by Vadim Lomshakov on 10/26/13.
// Copyright (c) 2013 spbau.
//


#include "BytecodeEmitter.h"

namespace mathvm {


  BytecodeEmitter::BytecodeEmitter():
  _code(0),
  _currentAstFunctionScope(0),
  _dynLibraryName("libc.dylib"),
  _dynLibraryHandle(0)
  {}

  BytecodeEmitter::~BytecodeEmitter() {}

  void BytecodeEmitter::visitPrintNode(PrintNode *printNode) {
    AstNode* op = 0;
    for (uint32_t i = 0; i < printNode->operands(); i++) {
      op = printNode->operandAt(i);
      op->visit(this);

      insnSet.print();
    }
  }

  void BytecodeEmitter::visitLoadByIndexNode(LoadByIndexNode *node) {
    AstVar const * ref = node->ref();
    LocationVar local = lookupCtxAndIdForVar(ref->name());

    // push arrayref on TOS
    if (local.first == currentScopeId())
      insnSet.loadVar(ref->type(), local.second);
    else
      insnSet.loadCtxVar(ref->type(), local.first, local.second);

    int32_t last = (int32_t)node->indexCount() - 1;
    for (int32_t i = 0; i < last; i++) {
      node->indexAt(i)->visit(this);
      insnSet.aaload();
    }

    node->indexAt(last)->visit(this);
    insnSet.aload(node->type());
  }

  void BytecodeEmitter::visitStoreByIndexNode(StoreByIndexNode *node) {
    // push arrayref and last idx on TOS
    {
      LoadByIndexNode* lhs = node->indexNode();
      AstVar const * ref = lhs->ref();
      LocationVar local = lookupCtxAndIdForVar(ref->name());

      // push arrayref on TOS
      if (local.first == currentScopeId())
        insnSet.loadVar(ref->type(), local.second);
      else
        insnSet.loadCtxVar(ref->type(), local.first, local.second);

      int32_t last = (int32_t)lhs->indexCount() - 1;
      for (int32_t i = 0; i < last; i++) {
        lhs->indexAt(i)->visit(this);
        insnSet.aaload();
      }

      lhs->indexAt(last)->visit(this);
    }

    // push rvalue on TOS
    node->value()->visit(this);

    if (node->op() == tINCRSET) {
      assert(false);
    } else if (node->op() == tDECRSET) {
      assert(false);
    }

    insnSet.astore();
  }

  void BytecodeEmitter::visitStoreNode(StoreNode *storeNode) {
    AstVar const *var = storeNode->var();
    BytecodeEmitter::LocationVar local = lookupCtxAndIdForVar(var->name());

    // push value of rhs on TOS
    AstBaseVisitor::visitStoreNode(storeNode);

    if (storeNode->op() != tASSIGN) {
      if (local.first == currentScopeId())
        insnSet.loadVar(var->type(), local.second);
      else
        insnSet.loadCtxVar(var->type(), local.first, local.second);
    }

    if (storeNode->op() == tINCRSET) {
      insnSet.add();
    } else if (storeNode->op() == tDECRSET) {
      insnSet.sub();
    }

    if (local.first == currentScopeId())
      insnSet.storeVar(var->type(), local.second);
    else
      insnSet.storeCtxVar(var->type(), local.first, local.second);
  }

  void BytecodeEmitter::visitNewArrayInstanceNode(NewArrayInstanceNode *node) {
    AstBaseVisitor::visitNewArrayInstanceNode(node);
    insnSet.newarray(node->refType(), node->dimsCount());
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
    ifNode->ifExpr()->visit(this);

    insnSet.prepareThenBranch();
    ifNode->thenBlock()->visit(this);

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

    node->right()->visit(this);
    node->left()->visit(this);

    switch (node->kind()) {
      case tADD:
        insnSet.add();
        break;
      case tSUB:
        insnSet.sub();
        break;
      case tMUL:
        insnSet.mul();
        break;
      case tDIV:
        insnSet.div();
        break;
      case tMOD:
        insnSet.mod();
        break;
      case tAOR:
        insnSet.aor();
        break;
      case tAAND:
        insnSet.aand();
        break;
      case tAXOR:
        insnSet.axor();
        break;
      case tEQ:
        insnSet.eq();
        break;
      case tNEQ:
        insnSet.neq();
        break;
      case tLT:
        insnSet.lt();
        break;
      case tGT:
        insnSet.gt();
        break;
      case tGE:
        insnSet.ge();
        break;
      case tLE:
        insnSet.le();
        break;
      case tAND:
        insnSet.band();
        break;
      case tOR:
        insnSet.bor();
        break;
      default:
        throw new logic_error("wrong operator kind " + string(tokenStr(node->kind())));
    }

  }

  void BytecodeEmitter::visitUnaryOpNode(UnaryOpNode *node) {

    AstBaseVisitor::visitUnaryOpNode(node);

    switch (node->kind()) {
      case tSUB:
        insnSet.neg();
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
    uint16_t id = _code->makeNativeFunction(node->nativeName(), node->nativeSignature(), getPointerOnFunction(node->nativeName()));
    insnSet.callNative(id, node->nativeSignature()[0].first);
  }

  void BytecodeEmitter::visitCallNode(CallNode *node) {
    // push args on TOS
    AstBaseVisitor::visitCallNode(node);

    insnSet.call(getFunctionIdByName(node->name()));
  }

  void BytecodeEmitter::visitDoubleLiteralNode(DoubleLiteralNode *node) {
    insnSet.load(node->literal());
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
    AstBaseVisitor::visitReturnNode(node);
    insnSet.riturn(currentBcFunction()->returnType());
  }

  void BytecodeEmitter::visitFunctionNode(FunctionNode *node) {
    BlockNode* blockNode = node->body();
    for (uint32_t i = 0; i < blockNode->nodes(); i++) {
      if (blockNode->nodeAt(i)->isCallNode()) {
        blockNode->nodeAt(i)->visit(this);

        //bad patch - clear type stack after unassigned call function
        AstFunction* foo = blockNode->scope()->lookupFunction(blockNode->nodeAt(i)->asCallNode()->name(), true);

        assert(foo != 0);
        if (foo->returnType() != VT_VOID)
          insnSet.skipRetVal();
      } else {
        blockNode->nodeAt(i)->visit(this);
      }
    }
  }

  void BytecodeEmitter::visitAstFunction(AstFunction  *function) {
    pushAstFunction(function);
    makeMappingFunctionParametersAndLocalsById();

    for (Scope::FunctionIterator i(_currentAstFunctionScope); i.hasNext(); ) {
      visitAstFunction(i.next());
    }

    function->node()->visit(this);

    Bytecode* bc = currentBcFunction()->bytecode();
    if (bc->getInsn(bc->length() - 1) != BC_RETURN) {
      (function->name() == AstFunction::top_name) ? insnSet.stop() : insnSet.riturn(currentBcFunction()->returnType());
    }

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
    for (Scope::VarIterator i(_currentAstFunctionScope); i.hasNext(); ++id) {
      v = i.next();
      _localsById.insert(make_pair(make_pair(v->name(), currentScopeId()), id));
    }

    currentBcFunction()->setLocalsNumber((uint16_t) _currentAstFunctionScope->variablesCount());
  }

  void BytecodeEmitter::makeMappingBlockLocals(Scope *scope) {
    uint16_t id = (uint16_t) (currentBcFunction()->localsNumber() + currentBcFunction()->parametersNumber());
    AstVar* v;

    for (Scope::VarIterator i(scope); i.hasNext(); ) {
      v = i.next();
      if (_localsById.insert(make_pair(make_pair(v->name(), currentScopeId()), id)).second)
        ++id;
    }
    currentBcFunction()->setLocalsNumber(id - currentBcFunction()->parametersNumber());
  }

  void BytecodeEmitter::pushAstFunction(AstFunction *astFunction) {
    uint16_t idFoo;
    TranslatedFunction* foo;

    // lazy creates function metadata
    if ((foo = _code->functionByName(astFunction->name())) == 0) {
      foo = new BytecodeFunction(astFunction);
      idFoo = _code->addFunction(foo);
      foo->setScopeId(idFoo);
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

  pair<uint16_t, TranslatedFunction*> BytecodeEmitter::getFunctionIdByName(string const &name) {
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
      foo->setScopeId(idFoo);
    } else {
      idFoo = foo->id();
    }

    return make_pair(idFoo, foo);
  }

  BytecodeEmitter &BytecodeEmitter::getInstance() {
    static BytecodeEmitter emitter;
    return emitter;
  }

  void BytecodeEmitter::setDynLibraryName(std::string const &library) {
    _dynLibraryName = library;
  }

  void* BytecodeEmitter::getPointerOnFunction(string const& name) {
    if (!_dynLibraryHandle) {
      assert(_dynLibraryName.size() != 0);
      _dynLibraryHandle = dlopen(_dynLibraryName.c_str(), RTLD_LAZY);
      if (!_dynLibraryHandle) {
        char const * msg = dlerror();
        throw std::runtime_error(msg ? msg : "[BytecodeEmitter]library not load");
      }
    }

    void* sym = dlsym(_dynLibraryHandle, name.c_str());
    if (!sym) {
      char const * msg = dlerror();
      throw std::runtime_error(msg ? msg : "[BytecodeEmitter]symbol not found");
    }

    return sym;
  }
}