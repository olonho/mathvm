//
// Created by Vadim Lomshakov on 10/27/13.
// Copyright (c) 2013 spbau. All rights reserved.
//

#ifndef __bcTemplates_H_
#define __bcTemplates_H_


#include <stdexcept>
#include <stack>
#include <cmath>
#include "ast.h"
#include "BytecodeEmitter.h"

namespace mathvm{

  class BcInstructionSet {
    Bytecode* _bc;
    static const int64_t FALSE = 0;

    stack<VarType> typeExpr;
    map<VarType, uint8_t> orderOnTypes;

    // for if template
    Label beginOfElseBlck;
    Label endOfElseBlck;
    bool hasElse;

    // for while and For template
    Label condition;
    Label block;

    uint16_t _idVarLoop;
    uint16_t* _ctx;
    VarType _typeVarLoop;
  public:
    BcInstructionSet() : _bc(0), typeExpr() {
      orderOnTypes[VarType::Double] = 3;
      orderOnTypes[VarType::Int] = 2;
      orderOnTypes[VarType::String] = 1;
    }

    void setContext(Bytecode* bc) { _bc = bc; }

    void stop() {
      emit(BC_STOP);
    }

    void tryConvertToLogic1() {
      assert(typeExpr.size() >= 1);

      VarType opType = typeExpr.top();
      typeExpr.pop();
      assert(opType != VT_INVALID && opType != VT_VOID);


      if (opType == VT_STRING)
        emit(BC_S2I);
      else if (opType == VT_DOUBLE)
        emit(BC_D2I);

      typeExpr.push(VT_INT);
    }



    void tryExpandConversion2(VarType maxType = VarType::Double) {
      assert(typeExpr.size() >= 2);
      assert(maxType == VT_DOUBLE || maxType == VT_INT);

      VarType op2Type = typeExpr.top();
      typeExpr.pop();
      assert(op2Type != VT_INVALID && op2Type != VT_VOID);
      VarType op1Type = typeExpr.top();
      typeExpr.pop();
      assert(op1Type != VT_INVALID && op1Type != VT_VOID);

      if (abs(orderOnTypes[op1Type] - orderOnTypes[op2Type]) == 2)
        throw std::logic_error("invalid operands to binary operator");

      if (orderOnTypes[maxType] < orderOnTypes[op2Type]
          || orderOnTypes[maxType] < orderOnTypes[op1Type])
        throw std::logic_error("invalid operands to binary operator");

      VarType resType = op1Type;

      if (orderOnTypes[op2Type] < orderOnTypes[op1Type]) {
        assert(op1Type == VT_DOUBLE || op1Type == VT_INT);
        if (op1Type == VT_DOUBLE)
          emit(BC_I2D);
        else
          emit(BC_S2I);
      }

      if (orderOnTypes[op1Type] < orderOnTypes[op2Type]) {
        assert(op2Type == VT_DOUBLE || op2Type == VT_INT);
        resType = op2Type;
        emit(BC_SWAP);
        if (op2Type == VT_DOUBLE)
          emit(BC_I2D);
        else
          emit(BC_S2I);
        emit(BC_SWAP);
      }

      typeExpr.push(resType);
    }

    void load(int64_t arg1) {
      typeExpr.push(VT_INT);

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
      typeExpr.push(VT_DOUBLE);

      // updating float pointing numbers hasn't occurred,
      // comparison of numbers is straightforward
      if (arg1 == 0.0) {
        emit(BC_DLOAD0);
        return;
      }

      if (arg1 == 1.0) {
        emit(BC_DLOAD1);
        return;
      }

      if (arg1 == -1.0)
        emit(BC_DLOADM1);
      else
        emit(BC_DLOAD, arg1);
    }

    void sload(uint16_t ref) {
      typeExpr.push(VT_STRING);

      emit(BC_SLOAD, ref);
    }

    void sload0() {
      typeExpr.push(VT_STRING);
      emit(BC_SLOAD0);
    }

    void aaload() {
      assert(typeExpr.top().tag() == VT_INT);
      typeExpr.pop();
      assert(typeExpr.top().tag() == VT_REF && typeExpr.top().dim() > 1);
      VarType type = typeExpr.top();
      typeExpr.pop();
      emit(BC_AALOAD);
      typeExpr.push(VarType::Arrayref(type.of(), type.dim() - 1));
    }

    void aload(VarType type) {
      assert(typeExpr.top().tag() == VT_INT);
      typeExpr.pop();
      assert((typeExpr.top().tag() == VT_REF && typeExpr.top().dim() > 0) || typeExpr.top().tag() == VT_INT || typeExpr.top().tag() == VT_DOUBLE);
      typeExpr.pop();
      typeExpr.push(type);

      switch (type.tag()) {
        case VT_INT:
          emit(BC_IALOAD);
          break;
        case VT_DOUBLE:
          emit(BC_DALOAD);
          break;
        case VT_REF:
          emit(BC_AALOAD);
          break;
        default:
          throw std::logic_error("wrong type for _ALOAD insn");
      }
    }

    void astore() {
      VarType type = typeExpr.top();
      typeExpr.pop();
      assert(typeExpr.top().tag() == VT_INT);
      typeExpr.pop();
      assert(typeExpr.top().tag() == VT_REF && typeExpr.top().dim() > 0);
      typeExpr.pop();

      // need check type

      switch (type.tag()) {
        case VT_INT:
          emit(BC_IASTORE);
          break;
        case VT_DOUBLE:
          emit(BC_DASTORE);
          break;
        case VT_REF:
          emit(BC_AASTORE);
          break;
        default:
          throw std::logic_error("wrong type for _ASTORE insn");
      }
    }

    void newarray(VarType type, uint16_t countDim) {
      for (uint16_t i = 0; i != countDim; ++i) {
        assert(typeExpr.top().tag() == VT_INT);
        typeExpr.pop();
      }

      typeExpr.push(type);

      if (countDim == 1) {
        switch (type.of().tag()) {
          case VT_INT:
            emit(BC_INEWARRAY);
            break;
          case VT_DOUBLE:
            emit(BC_DNEWARRAY);
            break;
          default:
            throw std::logic_error("wrong type for NEWARRAY insn");
        }
      } else if (countDim > 1) {
        switch (type.of().tag()) {
          case VT_INT:
            emit(BC_IMULTIANEWARRYA, countDim);
            break;
          case VT_DOUBLE:
            emit(BC_DMULTIANEWARRYA, countDim);
            break;
          default:
            throw std::logic_error("wrong type for MULTIANEWARRAY insn");
        }
      } else {
        assert(false);
      }
    }

    void loadVar(VarType type, uint16_t id) {
      typeExpr.push(type);

      //TODO use 0,1,...
      switch (type.tag()) {
        case VT_INT:
          emit(BC_LOADIVAR, id);
          break;
        case VT_DOUBLE:
          emit(BC_LOADDVAR, id);
          break;
        case VT_STRING:
          emit(BC_LOADSVAR, id);
          break;
        case VT_REF:
          emit(BC_LOADAVAR, id);
          break;
        default:
          throw std::logic_error("wrong type for LOADVAR insn");
      }
    }

    void loadCtxVar(VarType type, uint16_t ctx, uint16_t id) {
      typeExpr.push(type);

      switch (type.tag()) {
        case VT_INT:
          emit(BC_LOADCTXIVAR, ctx, id);
          break;
        case VT_DOUBLE:
          emit(BC_LOADCTXDVAR, ctx, id);
          break;
        case VT_STRING:
          emit(BC_LOADCTXSVAR, ctx, id);
          break;
        case VT_REF:
          emit(BC_LOADCTXAVAR, ctx, id);
          break;
        default:
          throw std::logic_error("wrong type for LOADCTXVAR insn");
      }
    }

    void storeVar(VarType type, uint16_t id) {
      // TODO conversion
      if (type != typeExpr.top())
        throw logic_error("wrong assignment");

      //TODO use 0,1,...
      switch (typeExpr.top().tag()) {
        case VT_INT:
          emit(BC_STOREIVAR, id);
          break;
        case VT_DOUBLE:
          emit(BC_STOREDVAR, id);
          break;
        case VT_STRING:
          emit(BC_STORESVAR, id);
          break;
        case VT_REF:
          emit(BC_STOREAVAR, id);
          break;
        default:
          throw std::logic_error("wrong type for STOREVAR insn");
      }
      typeExpr.pop();
    }

    void storeCtxVar(VarType type, uint16_t ctx, uint16_t id) {
      // TODO conversion
      if (type != typeExpr.top())
        throw logic_error("wrong assignment");

      switch (typeExpr.top().tag()) {
        case VT_INT:
          emit(BC_STORECTXIVAR, ctx, id);
          break;
        case VT_DOUBLE:
          emit(BC_STORECTXDVAR, ctx, id);
          break;
        case VT_STRING:
          emit(BC_STORECTXSVAR, ctx, id);
          break;
        case VT_REF:
          emit(BC_STORECTXAVAR, ctx, id);
          break;
        default:
          throw std::logic_error("wrong type for STORECTXVAR insn");
      }
      typeExpr.pop();
    }

    void print() {
      VarType type = typeExpr.top();
      typeExpr.pop();
      switch (type.tag()) {
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

    void call(pair<uint16_t, TranslatedFunction*> foo) {
      for (int i = foo.second->parametersNumber() - 1; i >= 0; --i) {
        if (typeExpr.top() != foo.second->parameterType(i))
          throw logic_error("mismatch type of argument function " + foo.second->name());
        typeExpr.pop();
      }

      emit(BC_CALL, foo.first);
      if (foo.second->returnType() != VT_VOID)
        typeExpr.push(foo.second->returnType());
    }

    void skipRetVal() {
      assert(!typeExpr.empty());
      typeExpr.pop();
      emit(BC_POP);
    }

    void callNative(uint16_t fId, VarType retType) {
      emit(BC_CALLNATIVE, fId);
      if (retType != VT_VOID)
        typeExpr.push(retType);
    }

    void riturn(VarType returnType) {
      // TODO conversion
      assert((typeExpr.size() == 1 && returnType != VT_VOID)
        || (typeExpr.empty() && returnType == VT_VOID));

      if (returnType != VT_VOID && returnType != typeExpr.top())
        throw logic_error("mismatch type of return expression");

      emit(BC_RETURN);
      if (returnType != VT_VOID)
        typeExpr.pop();
    }

    void add() {
      tryExpandConversion2();

      switch (typeExpr.top().tag()) {
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

    void sub() {
      tryExpandConversion2();

      switch (typeExpr.top().tag()) {
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

    void mul() {
      tryExpandConversion2();

      switch (typeExpr.top().tag()) {
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

    void div() {
      tryExpandConversion2();

      switch (typeExpr.top().tag()) {
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

    void mod() {
      tryExpandConversion2(VarType::Int);

      switch (typeExpr.top().tag()) {
        case VT_INT:
          emit(BC_IMOD);
          break;
        default:
          throw std::logic_error("wrong type for MOD insn");
      }
    }

    void aor() {
      tryExpandConversion2(VarType::Int);

      switch (typeExpr.top().tag()) {
        case VT_INT:
          emit(BC_IAOR);
          break;
        default:
          throw std::logic_error("wrong type for AOR insn");
      }
    }

    void aand() {
      tryExpandConversion2(VarType::Int);

      switch (typeExpr.top().tag()) {
        case VT_INT:
          emit(BC_IAAND);
          break;
        default:
          throw std::logic_error("wrong type for AAND insn");
      }
    }

    void axor() {
      tryExpandConversion2(VarType::Int);

      switch (typeExpr.top().tag()) {
        case VT_INT:
          emit(BC_IAXOR);
          break;
        default:
          throw std::logic_error("wrong type for AXOR insn");
      }
    }

    void cmp() {
      tryExpandConversion2();

      switch (typeExpr.top().tag()) {
        case VT_INT:
          emit(BC_ICMP);
          break;
        case VT_DOUBLE:
          emit(BC_DCMP);

          typeExpr.pop();
          typeExpr.push(VT_INT);
          break;
        default:
          throw std::logic_error("wrong type for CMP insn");
      }
    }

    void neg() {
      switch (typeExpr.top().tag()) {
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


    // let {0} - false, {1, -1} - true

    // -1 -> -2 -> 0
    //  0 -> -1 -> -1
    //  1 ->  0 -> 0
    void eq() {
      cmp();
      emit(BC_ILOADM1);
      emit(BC_IADD);
      emit(BC_ILOAD, (int64_t)2);
//      load((int64_t)2);
      emit(BC_SWAP);
      emit(BC_IMOD);
    }

    // -1
    //  0
    //  1
    void neq() {
      cmp();
    }

    // -1 ->  1 -> 0
    //  0 ->  2 -> 0
    //  1 ->  3 -> 1
    // notes: upper > lower
    void gt() {
      cmp();
      emit(BC_ILOAD, (int64_t)2);
//      load((int64_t)2);
      emit(BC_IADD);
      emit(BC_ILOAD, (int64_t)3);
//      load((int64_t)3);
      emit(BC_SWAP);
      emit(BC_IDIV);
    }


    // -1 ->  -3 -> -1
    //  0 ->  -2 -> 0
    //  1 ->  -1 -> 0
    // notes: upper < lower
    void lt() {
      cmp();
      emit(BC_ILOAD, (int64_t)-2);
//      load((int64_t)-2);
      emit(BC_IADD);
      emit(BC_ILOAD, (int64_t)3);
//      load((int64_t)3);
      emit(BC_SWAP);
      emit(BC_IDIV);
    }

    // -1 ->  0
    //  0 ->  1
    //  1 ->  2
    // notes: upper => lower
    void ge() {
      cmp();
      emit(BC_ILOAD1);
      emit(BC_IADD);
    }

    // -1 -> -2
    //  0 -> -1
    //  1 ->  0
    // notes: upper <= lower
    void le() {
      cmp();
      emit(BC_ILOADM1);
      emit(BC_IADD);
    }

    // is lazy
    void bor() {
      emit(BC_SWAP);

      prepareThenBranch();
      emit(BC_POP);
      emit(BC_ILOAD1);
      prepareElseBranch();
      // second expr convert to logic and return it
      tryConvertToLogic1();
      buildIf();
    }

    void band() {
      emit(BC_SWAP);

      prepareThenBranch();
      // second expr convert to logic and return it
      tryConvertToLogic1();
      prepareElseBranch();
      emit(BC_POP);
      emit(BC_ILOAD0);
      buildIf();
    }

    // -1 -> -2 ->  0
    //  0 -> -1 -> -1
    //  1 ->  0 ->  0
    void bnot() {
      tryConvertToLogic1();

      emit(BC_ILOADM1);
      emit(BC_IADD);
      emit(BC_ILOAD, (int64_t)2);
//      load((int64_t)2);
      emit(BC_SWAP);
      emit(BC_IMOD);
    }

//-------------ControlFLow-----------------------//

    // template if:
    //        iload0
    //        ificmpe
    //        then
    //        JA
    //        else

    void prepareThenBranch() {
      tryConvertToLogic1();

      beginOfElseBlck = Label(_bc);
      endOfElseBlck = Label(_bc);
      hasElse = false;

      load(FALSE);
      typeExpr.pop();
      typeExpr.pop();
      _bc->addBranch(BC_IFICMPE, beginOfElseBlck);

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
    //        ificmpne $b


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
      tryConvertToLogic1();
      load(FALSE);
      assert(typeExpr.size() == 2);
      typeExpr.pop();
      typeExpr.pop();
      _bc->addBranch(BC_IFICMPNE, block);
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
        loadVar(_typeVarLoop, _idVarLoop);
      } else {
        loadCtxVar(_typeVarLoop, _idVarLoop, *_ctx);
      }

      add();

      if (_ctx == 0) {
        storeVar(_typeVarLoop, _idVarLoop);
      } else {
        storeCtxVar(_typeVarLoop, _idVarLoop, *_ctx);
      }

      _bc->bind(condition);

      // prepare condition
      if (_ctx == 0) {
        loadVar(_typeVarLoop, _idVarLoop);
      } else {
        loadCtxVar(_typeVarLoop, _idVarLoop, *_ctx);
      }
    }

    void buildFor() {
      assert(typeExpr.size() == 2);
      typeExpr.pop();
      typeExpr.pop();
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
