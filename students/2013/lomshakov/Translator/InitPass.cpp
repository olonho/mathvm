//
// Created by Vadim Lomshakov on 15/12/13.
// Copyright (c) 2013 spbau. All rights reserved.
//

#include "InitPass.h"

namespace mathvm {

  void InitPass::visitAstFunction(AstFunction *astFunc) {
    Scope* bodyScope = astFunc->node()->body()->scope();
    markAsVisited(astFunc->scope());

    MachCodeFunc* func = 0;
    if ((func = _code->functionByName(astFunc->name())) == 0) {
      func = new MachCodeFunc(astFunc);
      _code->addFunction(func);
    }
    _funcStack.push(func);

    func->setDepth(_depthCnt);
    ++_depthCnt;



    // visit function parameters
    for (Scope::VarIterator it(astFunc->scope()); it.hasNext(); ) {
      initVarInfo(it.next(), astFunc->scope());
    }
    // visit top local vars
    for (Scope::VarIterator it(bodyScope); it.hasNext(); ) {
      initVarInfo(it.next(), bodyScope);
    }

    // visit ast function declared into body
    for (Scope::FunctionIterator it(bodyScope); it.hasNext(); ) {
      visitAstFunction(it.next());
    }


    // visit other stmt blocks(Scopes) into this function
    for (uint32_t i = 0; i != bodyScope->childScopeNumber(); ++i) {
      Scope* child = bodyScope->childScopeAt(i);
      if (!isVisited(child)) {
        visitBlockScope(child);
      }
    }


    --_depthCnt;
    _funcStack.pop();

    _sequenceFunctionForGen.push_back(astFunc->node());
  }

  void InitPass::visitBlockScope(Scope *scope) {
    markAsVisited(scope);

    // visit top local vars
    for (Scope::VarIterator it(scope); it.hasNext(); ) {
      initVarInfo(it.next(), scope);
    }

    // visit ast function declared into this block
    for (Scope::FunctionIterator it(scope); it.hasNext(); ) {
      visitAstFunction(it.next());
    }


    // visit other stmt blocks(Scopes) into this function
    for (uint32_t i = 0; i != scope->childScopeNumber(); ++i) {
      Scope* child = scope->childScopeAt(i);
      if (!isVisited(child)) {
        visitBlockScope(child);
      }
    }
  }

  void InitPass::initVarInfo(AstVar *var, Scope *sc) {
    VarId key = make_pair(var->name(), getScopeUID(sc));
    MachCodeFunc* func = _funcStack.top();

    assert(_addressByVar.count(key) == 0);
    assert(func->getVStackOffset() >= 2);

    uint32_t varOffset = func->getVStackOffset();
    func->setVStackOffset(varOffset + 1);

    VarAddress addr = make_pair(varOffset, func->getDepth());
    _addressByVar[key] = addr;
  }

  void InitPass::doInit() {
    visitAstFunction(_top);
  }

  InitPass::~InitPass() {

  }
}