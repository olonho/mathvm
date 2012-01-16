#pragma once 

#incluce "ast.h"
#include "mathvm.h"

#include "VarTable.h"

struct FunctionInfo {
	std::string _name;
	uint16_t _id;
	
	FunctionInfo() : _name(""), _id(0) {}
};

struct VarInfo {
	uint16_t _functionId;
	uint16_t _id;
};

class ScopeWrapper {
	bool _isFunction;
	struct FunctionInfo _functionInfo;
	bool _funcScopeInitialized;
		
	ScopeWrapper* _parentScope;
	VarTable *_varsTable;
			
	public:
				
		ScopeWrapper(ScopeWrapper *parent = 0, bool isFunction, const std::string &functionName = "", isInitialized = false, mathvm::Code *code=0) : 
			_isFunction(isFunction),
			_funcScopeInitialized(isInitialized),
			_parentScope(parent)
		{	
			int startIndex = 1;
			if (isFunction) {
				mathvm::TranslatedFunction *func = code->functionByName(functionName);
				_functionInfo._id = func->id();
				_functionInfo._name = func->name();
			} else {
				_functionInfo = parent->_functionInfo;
			}
			if (parent) startIndex += _parent->getVarsCount();
			_varsTable = new VarTable(startIndex);
		}
		
		~FunctionScopeWrapper() {
			delete _varsTable;
		}
		
		struct FunctionInfo getFunctionInfo() { return _functionInfo; }
		
		struct VarInfo getVarIdByName(const std::string& varName) {
			VarInfo var;
			bool found = false;
			uint16_t varId = 0;
			ScopeWrapper *searchScope = this;
			while (!found && searchScope) {
				varId=searchScope->_varsTable->tryFindIdByName(varName);
				if (varId) {
					var._functionId = searchScope->_functionInfo._id;
					var._id = varId;
					found = true;
				} else {
					searchScope = searchScope->_parentScope;
				}
			}
			return var;
			
		}
			
		uint16_t addVar(const string& varName) {
			_varsTable->addVar(varName);
		}
		
		uint16_t getVarsCount() {
			int count = _varsTable->varsCount();
			ScopeWrapper *parent = _parentScope;
			while (parent) {
				count += parent->_varsTable->size();
				parent = parent->_parentScope;
			}
			return count;
		}
		
		bool functionScopeInitialized() { return _funcScopeInitialized; }
		
		void setInitialized(bool initialized) { _funcScopeInitialized = initialized; }
		
		Scope* getParentScope () { return _parentScope; }

};
