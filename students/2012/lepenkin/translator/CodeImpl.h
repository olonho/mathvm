/*
 * CodeImpl.h
 *
 *  Created on: Dec 18, 2012
 *      Author: yarik
 */

#ifndef CODEIMPL_H_
#define CODEIMPL_H_


#include <mathvm.h>
#include <ast.h>
#include <stack>
#include <map>
#include <string>
#include <stdlib.h>




using std::string;


using namespace mathvm;



struct FunInfo
{
	FunInfo(uint16_t id)
	    : _id(id)
	{
	}

	const uint16_t _id;
};


struct VarInfo
{
   	VarInfo(uint16_t context, uint16_t id)
   	    : _context(context)
   	    , _id(id)
   	{
   	}

   	const uint16_t _context;
   	const uint16_t _id;
};



class SmartCode : public Code {


public:

	SmartCode();
	virtual ~SmartCode();

	Bytecode* currentBytecode();

	void pushFunction(BytecodeFunction* fun);
	void popFunction();
	uint16_t getTopFunctionId();

	void pushScope();
	void popScope();

    BytecodeFunction* declareFunctionInCurrentScope(FunctionNode* fun);
	VarInfo* declareVarInCurrentScope(AstVar* var);

    VarInfo* getVarInfoByName(const string& name);
    FunInfo* getFunIdByName(const string& name);

	Status* execute(vector<Var*>& vars);

private:


    stack<BytecodeFunction*> _funs;
    stack<Scope*> _scopes;
    map<uint16_t, uint16_t> _functionScope;

    Scope* getTopScope()
    {
        return (_scopes.size() > 0) ? _scopes.top() : 0;
    }



    VarInfo* declareVarAndGetInfo(const string& name, VarType type) {
        bool ok = getTopScope()->declareVariable(name, type);
        if (!ok)
        {
            cout << "Variable with the same name in scope: not allowed" << endl;
        	exit(EXIT_FAILURE);
        }
        uint16_t top = getTopFunctionId();
        uint16_t id = _functionScope[top]++;
        return new VarInfo(top, id);
    }


};







#endif /* CODEIMPL_H_ */
