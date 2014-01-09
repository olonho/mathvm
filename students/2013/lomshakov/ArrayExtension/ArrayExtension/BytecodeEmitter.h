//
// Created by Vadim Lomshakov on 10/26/13.
// Copyright (c) 2013 spbau. All rights reserved.
//



#ifndef __bcEmitter_H_
#define __bcEmitter_H_

#include <deque>
#include <map>
#include <dlfcn.h>

#include "visitors.h"
#include "BcInstructionSet.h"

namespace mathvm {


  class BytecodeEmitter: public AstBaseVisitor {
    typedef map<pair<string, uint16_t>, uint16_t> LocalsMap; // local and scopeId by local Id
    typedef pair<uint16_t, uint16_t> LocationVar;
    Code* _code;
    LocalsMap _localsById;

    deque<uint16_t> _functionId; // same as scopeId
    Scope* _currentAstFunctionScope;

    BcInstructionSet insnSet;

    std::string _dynLibraryName;
    void* _dynLibraryHandle;

    BytecodeEmitter();
    BytecodeEmitter(BytecodeEmitter&);
    BytecodeEmitter& operator=(BytecodeEmitter&);
  public:
    static BytecodeEmitter& getInstance();

    void setDynLibraryName(std::string const& library);

    void emitCode(AstFunction* topLevel, Code* code);

    void visitAstFunction(AstFunction *function);

#define VISITOR_FUNCTION(type, name)            \
    virtual void visit##type(type* node);

    FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION

    virtual ~BytecodeEmitter();

  private:

    void* getPointerOnFunction(string const& name);


    LocationVar lookupCtxAndIdForVar(string const &name);
    bool isLocalVar(string const& name, uint16_t scopeId) { return _localsById.count(make_pair(name, scopeId)) != 0; };
    Scope* currentAstScope() { return _currentAstFunctionScope; }
    BytecodeFunction* currentBcFunction() { return static_cast<BytecodeFunction*>(_code->functionById(_functionId.front())); }
    uint16_t currentScopeId() { return _functionId.front(); }

    void makeMappingFunctionParametersAndLocalsById();
    void makeMappingBlockLocals(Scope *);

    void pushAstFunction(AstFunction* );
    void popAstFunction();

    pair<uint16_t, TranslatedFunction*> getFunctionIdByName(string const &name);
  };

}
#endif //__bcEmitter_H_
