#pragma once 

#include "ast.h"
#include "mathvm.h"

#include "VarTable.h"

struct FunctionInfo {
	std::string _name;
	uint16_t _id;
	uint16_t _parametersCount;
	
	FunctionInfo() : _name(""), _id(0), _parametersCount(0) {}
};

struct VarInfo {
	uint16_t _functionId;
	uint16_t _id;

	VarInfo() : _functionId(0), _id(0) {}
};

class ScopeWrapper {
	bool _isFunction;
	struct FunctionInfo _functionInfo;
	bool _funcScopeInitialized;
		
	ScopeWrapper* _parentScope;
	VarTable *_varsTable;
			
	public:
				
		ScopeWrapper(ScopeWrapper *parent = 0, bool isFunction = false, const std::string &functionName = "", bool isInitialized = false, mathvm::Code *code=0) : 
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
			if (parent) startIndex += _parentScope->getVarsCount();
			_varsTable = new VarTable(startIndex);
		}
		
		~ScopeWrapper() {
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
			return _varsTable->addVar(varName);
		}
		
		uint16_t getVarsCount() {
			int count = _varsTable->varsCount();
			ScopeWrapper *parent = _parentScope;
			while (parent) {
				count += parent->_varsTable->varsCount();
				parent = parent->_parentScope;
			}
			return count;
		}
		
		bool functionScopeInitialized() { return _funcScopeInitialized; }
		
		void setInitialized(bool initialized) { _funcScopeInitialized = initialized; }
		
		ScopeWrapper* getParentScope () { return _parentScope; }

		void setParametersNumber(uint16_t count) { _functionInfo._parametersCount = count; }

};
