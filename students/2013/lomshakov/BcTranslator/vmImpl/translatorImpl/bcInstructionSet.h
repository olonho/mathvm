//
// Created by Vadim Lomshakov on 10/27/13.
// Copyright (c) 2013 spbau. All rights reserved.
//

#ifndef __bcTemplates_H_
#define __bcTemplates_H_


#include <stdexcept>
#include "ast.h"
#include "bcEmitter.h"

namespace mathvm{

  class BcInstructionSet {
    Bytecode* _bc;
    const int64_t TRUE = 1;
    const int64_t FALSE = 0;


    // for if template
    Label beginOfElseBlck;
    Label endOfElseBlck;
    bool hasElse;

    // for while and For template
    Label condition;
    Label block;

    uint16_t _idVarLoop;
    uint16_t*_ctx;
    VarType _typeVarLoop;
  public:
    BcInstructionSet() : _bc(0) {}

    void setContext(Bytecode* bc) { _bc = bc; }

    void load(int64_t arg1) {
      switch (arg1) {
        case 0:
          emit(BC_ILOAD0);
          break;
        case 1:
          emit(BC_ILOAD1);
          break;
        case -1:
          emit(BC_ILOADM1);
          break;
        default:
          emit(BC_ILOAD, arg1);
      }
    }

    void load(double arg1) {
      // updating float pointing numbers hasn't occurred,
      // comparison of numbers is straightforward
      if (arg1 == 0.0)
        emit(BC_DLOAD0);

      if (arg1 == 0.0)
        emit(BC_DLOAD1);

      if (arg1 == -1.0)
        emit(BC_DLOADM1);
      else
        emit(BC_DLOAD, arg1);
    }

    void sload(uint16_t ref) {
      emit(BC_SLOAD, ref);
    }

    void sload0() {
      emit(BC_SLOAD0);
    }

    void loadVar(VarType type, uint16_t id) {
      //TODO use 0,1,...
      switch (type) {
        case VT_INT:
          emit(BC_LOADIVAR, id);
          break;
        case VT_DOUBLE:
          emit(BC_LOADDVAR, id);
          break;
        case VT_STRING:
          emit(BC_LOADSVAR, id);
          break;
        default:
          throw std::logic_error("wrong type for LOADVAR insn");
      }
    }

    void loadCtxVar(VarType type, uint16_t ctx, uint16_t id) {
      switch (type) {
        case VT_INT:
          emit(BC_LOADCTXIVAR, ctx, id);
          break;
        case VT_DOUBLE:
          emit(BC_LOADCTXDVAR, ctx, id);
          break;
        case VT_STRING:
          emit(BC_LOADCTXSVAR, ctx, id);
          break;
        default:
          throw std::logic_error("wrong type for LOADCTXVAR insn");
      }
    }

    void storeVar(VarType type, uint16_t id) {
      //TODO use 0,1,...
      switch (type) {
        case VT_INT:
          emit(BC_STOREIVAR, id);
          break;
        case VT_DOUBLE:
          emit(BC_STOREDVAR, id);
          break;
        case VT_STRING:
          emit(BC_STORESVAR, id);
          break;
        default:
          throw std::logic_error("wrong type for STOREVAR insn");
      }
    }

    void storeCtxVar(VarType type, uint16_t ctx, uint16_t id) {
      switch (type) {
        case VT_INT:
          emit(BC_STORECTXIVAR, ctx, id);
          break;
        case VT_DOUBLE:
          emit(BC_STORECTXDVAR, ctx, id);
          break;
        case VT_STRING:
          emit(BC_STORECTXSVAR, ctx, id);
          break;
        default:
          throw std::logic_error("wrong type for STORECTXVAR insn");
      }
    }

    void print(VarType type) {
      switch (type) {
        case VT_INT:
          emit(BC_IPRINT);
          break;
        case VT_DOUBLE:
          emit(BC_DPRINT);
          break;
        case VT_STRING:
          emit(BC_SPRINT);
          break;
        default:
          throw std::logic_error("wrong type for PRINT insn");
      }
    }

    void call(uint16_t fId) {
      emit(BC_CALL, fId);
    }

    void callNative(uint16_t fId) {
      emit(BC_CALLNATIVE, fId);
    }

    void rëturn() {
      emit(BC_RETURN);
    }

    void add(VarType type) {
      switch (type) {
        case VT_INT:
          emit(BC_IADD);
          break;
        case VT_DOUBLE:
          emit(BC_DADD);
          break;
        default:
          throw std::logic_error("wrong type for ADD insn");
      }
    }

    void sub(VarType type) {
      switch (type) {
        case VT_INT:
          emit(BC_ISUB);
          break;
        case VT_DOUBLE:
          emit(BC_DSUB);
          break;
        default:
          throw std::logic_error("wrong type for SUB insn");
      }
    }

    void mul(VarType type) {
      switch (type) {
        case VT_INT:
          emit(BC_IMUL);
          break;
        case VT_DOUBLE:
          emit(BC_DMUL);
          break;
        default:
          throw std::logic_error("wrong type for MUL insn");
      }
    }

    void div(VarType type) {
      switch (type) {
        case VT_INT:
          emit(BC_IDIV);
          break;
        case VT_DOUBLE:
          emit(BC_DDIV);
          break;
        default:
          throw std::logic_error("wrong type for DIV insn");
      }
    }

    void mod(VarType type) {
      switch (type) {
        case VT_INT:
          emit(BC_IMOD);
          break;
        default:
          throw std::logic_error("wrong type for MOD insn");
      }
    }

    void aor(VarType type) {
      switch (type) {
        case VT_INT:
          emit(BC_IAOR);
          break;
        default:
          throw std::logic_error("wrong type for MOD insn");
      }
    }

    void aand(VarType type) {
      switch (type) {
        case VT_INT:
          emit(BC_IAAND);
          break;
        default:
          throw std::logic_error("wrong type for MOD insn");
      }
    }

    void axor(VarType type) {
      switch (type) {
        case VT_INT:
          emit(BC_IAXOR);
          break;
        default:
          throw std::logic_error("wrong type for MOD insn");
      }
    }

    void cmp(VarType type) {
      switch (type) {
        case VT_INT:
          emit(BC_ICMP);
          break;
        case VT_DOUBLE:
          emit(BC_DCMP);
          break;
        default:
          throw std::logic_error("wrong type for ADD insn");
      }
    }

    void neg(VarType type) {
      switch (type) {
        case VT_INT:
          emit(BC_INEG);
          break;
        case VT_DOUBLE:
          emit(BC_DNEG);
          break;
        default:
          throw std::logic_error("wrong type for ADD insn");
      }
    }

    void eq(VarType type) {
      Label startOfTrueBlck;
      _bc->addBranch(BC_IFICMPE, startOfTrueBlck);

      load(FALSE);
      Label endOfFalseBlck;
      _bc->addBranch(BC_JA, endOfFalseBlck);

      _bc->bind(startOfTrueBlck);
      load(TRUE);
      _bc->bind(endOfFalseBlck);
    }

    void neq(VarType type) {
      Label startOfTrueBlck;
      _bc->addBranch(BC_IFICMPNE, startOfTrueBlck);
      load(FALSE);

      Label endOfFalseBlck;
      _bc->addBranch(BC_JA, endOfFalseBlck);

      _bc->bind(startOfTrueBlck);
      load(TRUE);
      _bc->bind(endOfFalseBlck);
    }

    // notes: lower < upper
    void lt(VarType type) {
      Label startOfTrueBlck;
      _bc->addBranch(BC_IFICMPG, startOfTrueBlck);
      load(FALSE);

      Label endOfFalseBlck;
      _bc->addBranch(BC_JA, endOfFalseBlck);

      _bc->bind(startOfTrueBlck);
      load(TRUE);
      _bc->bind(endOfFalseBlck);
    }

    // notes: lower > upper
    void gt(VarType type) {
      Label startOfTrueBlck;
      _bc->addBranch(BC_IFICMPL, startOfTrueBlck);
      load(FALSE);

      Label endOfFalseBlck;
      _bc->addBranch(BC_JA, endOfFalseBlck);

      _bc->bind(startOfTrueBlck);
      load(TRUE);
      _bc->bind(endOfFalseBlck);
    }

    // notes: lower <= upper
    void le(VarType type) {
      Label startOfTrueBlck;
      _bc->addBranch(BC_IFICMPGE, startOfTrueBlck);
      load(FALSE);

      Label endOfFalseBlck;
      _bc->addBranch(BC_JA, endOfFalseBlck);

      _bc->bind(startOfTrueBlck);
      load(TRUE);
      _bc->bind(endOfFalseBlck);
    }

    // notes: lower >= upper
    void ge(VarType type) {
      Label startOfTrueBlck;
      _bc->addBranch(BC_IFICMPLE, startOfTrueBlck);
      load(FALSE);

      Label endOfFalseBlck;
      _bc->addBranch(BC_JA, endOfFalseBlck);

      _bc->bind(startOfTrueBlck);
      load(TRUE);
      _bc->bind(endOfFalseBlck);
    }

    // is lazy
    void bor() {

    }

    void band() {

    }

    // expect 0 or 1 on TOS
    void bnot() {
      load((int64_t)1);
      add(VT_INT);

      load((int64_t)2);
      emit(BC_SWAP);

      mod(VT_INT);
    }

//-------------ControlFLow-----------------------//

    // template if:
    //        ificmpge
    //        then
    //        JA
    //        else

    void prepareThenBranch() {
      beginOfElseBlck = Label(_bc);
      endOfElseBlck = Label(_bc);
      hasElse = false;

      load(FALSE);
      _bc->addBranch(BC_IFICMPGE, beginOfElseBlck);

    }

    void prepareElseBranch() {
      hasElse = true;
      _bc->addBranch(BC_JA, endOfElseBlck);
      _bc->bind(beginOfElseBlck);
    }

    void buildIf() {
      if (hasElse) {
        _bc->bind(endOfElseBlck);
      } else {
        _bc->bind(beginOfElseBlck);
      }
    }
//-----------------------------------------------//

    // template while:
    //        JA $c
    //    $b: ...
    //    $c: expr
    //        iload0
    //        ificmpl $b


    void prepareWhileBlock() {
      condition = Label(_bc);
      block = Label(_bc);

      _bc->addBranch(BC_JA, condition);

      _bc->bind(block);
    }

    void prepareWhileCondition() {
      _bc->bind(condition);
    }

    void buildWhile() {
      load(FALSE);
      _bc->addBranch(BC_IFICMPL, block);
    }
//-----------------------------------------------//

    // template for:
    //        expr1
    //        storevar id
    //        JA $c
    //    $b: ...
    //        iload1
    //        loadvar id
    //        iadd
    //    $c: loadvar id
    //        expr2
    //        ificmpge $b

    void prepareForVar(VarType typeVarLoop, uint16_t idVarLoop, uint16_t* ctx = 0) {
      condition = Label(_bc);
      block = Label(_bc);
      _ctx = ctx;
      _idVarLoop = idVarLoop;
      _typeVarLoop = typeVarLoop;

      if (_ctx == 0) {
        storeVar(_typeVarLoop, _idVarLoop);
      } else {
        storeCtxVar(_typeVarLoop, _idVarLoop, *_ctx);
      }

      _bc->addBranch(BC_JA, condition);

      _bc->bind(block);
    }

    void prepareForCondition() {

      // increase var loop
      _typeVarLoop == VT_INT ? load((int64_t)1) : load(1.0);

      if (_ctx == 0) {
        storeVar(_typeVarLoop, _idVarLoop);
      } else {
        storeCtxVar(_typeVarLoop, _idVarLoop, *_ctx);
      }

      add(_typeVarLoop);

      _bc->bind(condition);

      // prepare condition
      if (_ctx == 0) {
        storeVar(_typeVarLoop, _idVarLoop);
      } else {
        storeCtxVar(_typeVarLoop, _idVarLoop, *_ctx);
      }
    }

    void buildFor() {
      _bc->addBranch(BC_IFICMPGE, block);
    }

  private:
    template<typename T>
    void emit(Instruction insn, T const arg1) {
      _bc->addInsn(insn);
      _bc->addTyped(arg1);
    }

    void emit(Instruction insn) {
      _bc->addInsn(insn);
    }

    template<typename T>
    void emit(Instruction insn, T const arg1, T const arg2) {
      _bc->addInsn(insn);
      _bc->addTyped(arg1);
      _bc->addTyped(arg2);
    }



    BcInstructionSet(BcInstructionSet&);
    BcInstructionSet& operator=(BcInstructionSet&);
  };




} //mathvm

#endif //__bcTemplates_H_
