//
// Created by Vadim Lomshakov on 15/12/13.
// Copyright (c) 2013 spbau. All rights reserved.
//


#ifndef __InitPass_H_
#define __InitPass_H_

#include "visitors.h"
#include "MachCode.h"
#include <map>
#include <stack>


namespace mathvm {



/* This pass by tree needs to do following actions:
 * - get depth function (use into display for local vars)
 * - make mapping from (varName, scopeUId) to (offset on stack, function depth)
 * - get sequence function nodes for generation code into MachineCodeGenerator
 * (Traversing scopes is like dfs)
 */

class InitPass {
  uint32_t _depthCnt;
  MachCode* _code;
  AstFunction* _top;

  stack<MachCodeFunc*> _funcStack;
  set<int64_t> _visitedScopes;

  VarAllocation& _addressByVar;
  vector<FunctionNode*>& _sequenceFunctionForGen;
public:

  InitPass(AstFunction* top, MachCode* code, VarAllocation& varAllocation, vector<FunctionNode*>& sequenceFuncs) :
    _depthCnt(0),
    _code(code),
    _top(top),
    _addressByVar(varAllocation),
    _sequenceFunctionForGen(sequenceFuncs) {

  }

  void doInit();

  virtual ~InitPass();
private:

  void visitAstFunction(AstFunction* astFunc);
  void visitBlockScope(Scope* scope);

  void initVarInfo(AstVar *var, Scope *sc);


  void markAsVisited(Scope* sc) { _visitedScopes.insert(getScopeUID(sc)); }
  bool isVisited(Scope* sc) { return _visitedScopes.count(getScopeUID(sc)) == 1; }
};

}

#endif //__InitPass_H_
