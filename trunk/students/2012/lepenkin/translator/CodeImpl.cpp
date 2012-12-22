/*
 * CodeImpl.cpp
 *
 *  Created on: Dec 18, 2012
 *      Author: yarik
 */

#include "CodeImpl.h"

SmartCode::SmartCode() {
}

SmartCode::~SmartCode() {
}


Bytecode* SmartCode::currentBytecode() {
    return _funs.top()->bytecode();
}

uint16_t SmartCode::getTopFunctionId() {
	return _funs.top()->id();
}



void SmartCode::pushFunction(BytecodeFunction* fun) {
   _funs.push(fun);
}


void SmartCode::popFunction() {
	if (_funs.size() == 0) {
		cout << "Poping out empty functions stack" << endl;
		exit(EXIT_FAILURE);
	}
	_funs.pop();
}


void SmartCode::pushScope() {
    _scopes.push(new Scope(getTopScope()));
}

void SmartCode::popScope() {
	if (_scopes.size() == 0) {
        cout << "Poping out empty stack" << endl;
        exit(EXIT_FAILURE);
	}
	_scopes.pop();
}



BytecodeFunction* SmartCode::declareFunctionInCurrentScope(FunctionNode* fun) {
	Scope* top = getTopScope();
	bool ok = top->declareFunction(fun);
	if (!ok)
	{
	    cout << "Function with the same name in scope: not allowed" << endl;
	    exit(EXIT_FAILURE);
	}
	AstFunction* declFun = top->lookupFunction(fun->name());

	BytecodeFunction* bfun = new BytecodeFunction(declFun);
    uint16_t id = addFunction(bfun);
    declFun->setInfo(new FunInfo(id));
    return bfun;
}


VarInfo* SmartCode::declareVarInCurrentScope(AstVar* var) {

	VarInfo* info = declareVarAndGetInfo(var->name(), var->type());
	AstVar* scopeVar = getTopScope()->lookupVariable(var->name());
	scopeVar->setInfo(info);

	return info;
}


VarInfo* SmartCode::getVarInfoByName(const string& name) {
    AstVar* var = getTopScope()->lookupVariable(name);
    return (VarInfo*)(var ? var->info() : 0);
}

FunInfo* SmartCode::getFunIdByName(const string& name) {
  	AstFunction* fun = getTopScope()->lookupFunction(name);
	return (FunInfo*)(fun ? fun->info() : 0);
}

Status* SmartCode::execute(vector<Var*>& vars)
{
	return 0;
}

