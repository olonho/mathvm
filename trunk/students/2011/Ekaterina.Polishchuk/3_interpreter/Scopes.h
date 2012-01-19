#ifndef SCOPES_H
#define SCOPES_H

#include "mathvm.h"
#include "ast.h"
#include "VarId.h"
#include "FunctionId.h"
#include "TranslationException.h"

class FunctionScope;

class BlockScope {
protected:
    FunctionScope * myParentFunction;
    mathvm::Scope * myScope;
    typedef std::map<std::string, VarId> VarIdMap;
    typedef std::map<std::string, FunctionId> FunctionIdMap;
    VarIdMap myVars;
    FunctionIdMap myFunctions;
public:
    BlockScope(mathvm::Scope* scope, FunctionScope* parentFunction);
    BlockScope(mathvm::Scope* scope);
    virtual ~BlockScope(){}
    bool varExists(std::string const & name) const;
    bool functionExists(std::string const & name) const;
    void declareVariable(std::string const & name);
    void declareFunction(FunctionId const & fid);
    VarId getVariableId(std::string const & name);
    mathvm::Scope const * scope() const {
        return myScope;
    }
    virtual bool isFunction() const {
        return false;
    }

    void declareVariables();
};

class FunctionScope: public BlockScope {
private:
    uint16_t myTotalVariablesNum;
    FunctionId myId;
public:	
    friend class BlockScope;
    FunctionScope(FunctionId const & id, mathvm::Scope * bodyScope);
    uint16_t getId() const {
	return myId.id;
    }
    mathvm::AstFunction* getAstFunction() const;
    uint16_t getTotalVariablesNum() const {
        return myTotalVariablesNum;
    }
    virtual bool isFunction() const {
        return true;
    }
};
#endif
