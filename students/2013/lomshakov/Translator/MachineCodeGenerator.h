//
// Created by Vadim Lomshakov on 13/12/13.
// Copyright (c) 2013 spbau. All rights reserved.
//


#include "visitors.h"
#include "MachCode.h"
#include "InitPass.h"
#include <stdexcept>
#include <stack>
#include <dlfcn.h>
#include <AsmJit/AsmJit.h>


#ifndef __MachineCodeGenerator_H_
#define __MachineCodeGenerator_H_

namespace mathvm {
  using namespace AsmJit;

  class MachineCodeGenerator : public AstBaseVisitor {
    AstFunction* _top;
    MachCode* _code;
    Compiler _;
    VarAllocation _addressByVar;

    // function context
    MachCodeFunc* _currGenFunc;
    stack<BaseVar> _stackOperand;
    int32_t _assignmentCnt; // if _assignmentCnt > 0; evaluation is assigned
    stack<VarType> _typeStack;
    GPVar divider;
  public:
    MachineCodeGenerator(AstFunction* top, MachCode* code) :
      _top(top),
      _code(code),
      _assignmentCnt(0)
    {
      _ccnv = CALL_CONV_DEFAULT;

      initLIBC();
    }

    void generate();

#define VISITOR_FUNCTION(type, name)            \
    virtual void visit##type(type* node);

    FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION

    virtual ~MachineCodeGenerator();
  private:
    // settigs Compiler:
    CALL_CONV _ccnv;

    void* _LIBC_Handle;
    char const* _LIBC_NAME;
    // using symbols from libc
    void* _printfSym;
  private:

    inline void initLIBC() {
      _LIBC_NAME = "libc.dylib";
      _LIBC_Handle = dlopen(_LIBC_NAME, RTLD_LAZY);
      if (!_LIBC_Handle) {
        char const * msg = dlerror();
        throw std::runtime_error(msg ? msg : "[MachineCodeGenerator]library not load");
      }

      _printfSym = initLIBCSym("printf");
    }

    inline void* initLIBCSym(string const& name) {
      void* sym = dlsym(_LIBC_Handle, name.c_str());
      if (!sym) {
        char const * msg = dlerror();
        throw std::runtime_error(msg ? msg : "[MachineCodeGenerator]symbol not found");
      }
      return sym;
    }

    inline FunctionDefinition makeDefinition(uint32_t countArgs) {
      FunctionBuilderX definition;
      for (size_t i = 0; i != countArgs; ++i)
        definition.addArgument<int64_t>();
      definition.setReturnValue<int64_t>();
      return definition;
    }

    inline void pushOperand(BaseVar const & var) {
      _stackOperand.push(var);
    }

    template<class T>
    inline T popOperand() {
      assert(_stackOperand.size() >= 1);
      T t = static_cast<T&>(_stackOperand.top());
      _stackOperand.pop();
      return t;
    }

    inline void pushType(VarType t) {
      assert(t == VT_DOUBLE || t == VT_STRING || t == VT_INT);
      _typeStack.push(t);
    }

    inline VarType popType() {
      assert(!_typeStack.empty());
      VarType t = _typeStack.top();
      _typeStack.pop();
      return t;
    }

    inline void checkTypeCompatibility(VarType l, VarType r) {
      if ((l == VT_DOUBLE || l == VT_INT)
        && (r == VT_DOUBLE || r == VT_INT))
        return;
      if (l == VT_STRING && r == VT_STRING)
        return;
      throw runtime_error("types aren't compatability in assignment");
    }

    inline BaseVar assignmentConversion(VarType leftType, VarType rightType) {
      checkTypeCompatibility(leftType, rightType);

      if (leftType != VT_DOUBLE) {

        if (rightType != VT_DOUBLE)
          return popOperand<GPVar>();
        else {
          GPVar tmp(_.newGP());
          _.cvtsd2si(tmp, popOperand<XMMVar>());
          return tmp;
        }
      } else {

        if (rightType == VT_DOUBLE)
          return popOperand<XMMVar>();
        else {
          XMMVar tmp(_.newXMM(VARIABLE_TYPE_XMM_1D));
          _.cvtsi2sd(tmp, popOperand<GPVar>());
          return tmp;
        }
      }
    }

    inline VarType binaryOpConversion() {
      VarType right = popType();
      VarType left = popType();
      checkTypeCompatibility(left, right);

      // stub
      assert(right == left);

      pushType(left);
      return left;
    }

    inline VarType binaryOpLConversion() {
      VarType right = popType();
      VarType left = popType();
      checkTypeCompatibility(left, right);

      // stub
      assert(right == left);

      pushType(VT_INT);
      return left;
    }

  private:

    void buildFunctionPrototypeBy(MachCodeFunc *func, FunctionBuilderX& defn);

    Mem getVarFromStack(AstVar const *astVar);

    void appendReturn();

    void vprolog();
    void vepilog();

    void prolog();
    void epilog();

    GPVar getVarAddressOnStack(AstVar const *astVar);
  private:
    // simple and stupid

    inline void genNeg() {
      VarType typeOp = popType();
      pushType(typeOp);
      if (typeOp == VT_INT) {
        GPVar op = popOperand<GPVar>();
        _.neg(op);
        pushOperand(op);
      } else if (typeOp == VT_DOUBLE) {
        double m = -1.0;
        int64_t i = *((int64_t*)&m);
        XMMVar minus1(_.newXMM());
        GPVar tmp(_.newGP());
        _.mov(tmp, Imm(i));
        _.movq(minus1, tmp);
        _.unuse(tmp);

        XMMVar op = popOperand<XMMVar>();
        _.mulsd(op, minus1);
        _.unuse(minus1);
        pushOperand(op);
      } else
        throw runtime_error("wrong type operand for unary '-'");
    }

    inline void genNot() {
      VarType typeOp = popType();
      pushType(typeOp);
      switch (typeOp) {
        case VT_INT: {
          AsmJit::Label zeroL = _.newLabel();
          AsmJit::Label exitL = _.newLabel();
          GPVar op = popOperand<GPVar>();
          _.cmp(op, 0);
          _.je(zeroL);

          _.mov(op, 0);
          _.jmp(exitL);

          _.bind(zeroL);
          _.mov(op, 1);

          _.bind(exitL);
          pushOperand(op);
          break;
        }
        default:
          throw runtime_error("wrong type operand for 'not'");
      }
    }

    inline void genAdd() {
      switch (binaryOpConversion()) {
        case VT_INT: {
          GPVar rOp = popOperand<GPVar>();
          _.add(rOp, popOperand<GPVar>());
          pushOperand(rOp);
          break;
        }
        case VT_DOUBLE: {
          XMMVar rOp = popOperand<XMMVar>();
          _.addsd(rOp, popOperand<XMMVar>());
          pushOperand(rOp);
          break;
        }
        default:
          throw runtime_error("wrong type operand for '+'");
      }
    }

    inline void genSub() {
      switch (binaryOpConversion()) {
        case VT_INT: {
          GPVar rOp = popOperand<GPVar>();
          GPVar lOp = popOperand<GPVar>();
          _.sub(lOp, rOp);
          pushOperand(lOp);
          break;
        }
        case VT_DOUBLE: {
          XMMVar rOp = popOperand<XMMVar>();
          XMMVar lOp = popOperand<XMMVar>();
          _.subsd(lOp, rOp);
          pushOperand(lOp);
          break;
        }
        default:
          throw runtime_error("wrong type operand for '-'");
      }
    }

    inline void genMul() {
      switch (binaryOpConversion()) {
        case VT_INT: {
          GPVar rOp = popOperand<GPVar>();
          _.imul(rOp, popOperand<GPVar>());
          pushOperand(rOp);
          break;
        }
        case VT_DOUBLE: {
          XMMVar rOp = popOperand<XMMVar>();
          _.mulsd(rOp, popOperand<XMMVar>());
          pushOperand(rOp);
          break;
        }
        default:
          throw runtime_error("wrong type operand for '+'");
      }
    }

    inline void genDiv() {
      switch (binaryOpConversion()) {
        case VT_INT: {
          GPVar rOp = popOperand<GPVar>();
          GPVar lOp = popOperand<GPVar>();
          _.mov(divider, rOp); // magick work around for bug in AsmJit
          GPVar tmp(_.newGP());
          _.xor_(tmp, tmp);
          _.idiv_lo_hi(lOp, tmp, divider);
          _.unuse(tmp);
          pushOperand(lOp);
          break;
        }
        case VT_DOUBLE: {
          XMMVar rOp = popOperand<XMMVar>();
          XMMVar lOp = popOperand<XMMVar>();
          _.divsd(lOp, rOp);
          pushOperand(lOp);
          break;
        }
        default:
          throw runtime_error("wrong type operand for '/'");
      }
    }

    inline void genMod() {
      switch (binaryOpConversion()) {
        case VT_INT: {
          GPVar rOp = popOperand<GPVar>();
          GPVar lOp = popOperand<GPVar>();
          _.mov(divider, rOp); // magick work around for bug in AsmJit
          GPVar tmp(_.newGP());
          _.xor_(tmp, tmp);
          _.idiv_lo_hi(lOp, tmp, divider);
          pushOperand(tmp);
          break;
        }
        default:
          throw runtime_error("wrong type operand for '%'");
      }
    }

    inline void genEq() {
      switch (binaryOpLConversion()) {
        case VT_INT: {
          AsmJit::Label equalL = _.newLabel();
          AsmJit::Label exitL = _.newLabel();
          GPVar rOp = popOperand<GPVar>();
          _.cmp(popOperand<GPVar>(), rOp);
          _.je(equalL);

          _.mov(rOp, Imm(0));
          _.jmp(exitL);

          _.bind(equalL);

          _.mov(rOp, Imm(1));

          _.bind(exitL);

          pushOperand(rOp);
          break;
        }
        case VT_DOUBLE: {
          AsmJit::Label equalL = _.newLabel();
          AsmJit::Label exitL = _.newLabel();
          GPVar res(_.newGP());
          _.comisd(popOperand<XMMVar>(), popOperand<XMMVar>());
          _.je(equalL);

          _.mov(res, Imm(0));
          _.jmp(exitL);

          _.bind(equalL);

          _.mov(res, Imm(1));

          _.bind(exitL);

          pushOperand(res);
          break;
        }
        default:
          throw runtime_error("wrong type operand for '=='");
      }
    }

    inline void genNeq() {
      switch (binaryOpLConversion()) {
        case VT_INT: {
          AsmJit::Label notEqualL = _.newLabel();
          AsmJit::Label exitL = _.newLabel();
          GPVar rOp = popOperand<GPVar>();
          _.cmp(popOperand<GPVar>(), rOp);
          _.jne(notEqualL);

          _.mov(rOp, Imm(0));
          _.jmp(exitL);

          _.bind(notEqualL);

          _.mov(rOp, Imm(1));

          _.bind(exitL);

          pushOperand(rOp);
          break;
        }
        case VT_DOUBLE: {
          AsmJit::Label notEqualL = _.newLabel();
          AsmJit::Label exitL = _.newLabel();
          GPVar res(_.newGP());
          _.comisd(popOperand<XMMVar>(), popOperand<XMMVar>());
          _.jne(notEqualL);

          _.mov(res, Imm(0));
          _.jmp(exitL);

          _.bind(notEqualL);

          _.mov(res, Imm(1));

          _.bind(exitL);

          pushOperand(res);
          break;
        }
        default:
          throw runtime_error("wrong type operand for '!='");
      }
    }

    inline void genLt() {
      switch (binaryOpLConversion()) {
        case VT_INT: {
          AsmJit::Label lessL = _.newLabel();
          AsmJit::Label exitL = _.newLabel();
          GPVar rOp = popOperand<GPVar>();
          _.cmp(popOperand<GPVar>(), rOp);
          _.jl(lessL);

          _.mov(rOp, Imm(0));
          _.jmp(exitL);

          _.bind(lessL);

          _.mov(rOp, Imm(1));

          _.bind(exitL);

          pushOperand(rOp);
          break;
        }
        case VT_DOUBLE: {
          AsmJit::Label lessL = _.newLabel();
          AsmJit::Label exitL = _.newLabel();
          GPVar res(_.newGP());
          _.comisd(popOperand<XMMVar>(), popOperand<XMMVar>());
          _.jl(lessL);

          _.mov(res, Imm(0));
          _.jmp(exitL);

          _.bind(lessL);

          _.mov(res, Imm(1));

          _.bind(exitL);

          pushOperand(res);
          break;
        }
        default:
          throw runtime_error("wrong type operand for '<'");
      }
    }

    inline void genGt() {
      switch (binaryOpLConversion()) {
        case VT_INT: {
          AsmJit::Label grL = _.newLabel();
          AsmJit::Label exitL = _.newLabel();
          GPVar rOp = popOperand<GPVar>();
          _.cmp(popOperand<GPVar>(), rOp);
          _.jg(grL);

          _.mov(rOp, Imm(0));
          _.jmp(exitL);

          _.bind(grL);

          _.mov(rOp, Imm(1));

          _.bind(exitL);

          pushOperand(rOp);
          break;
        }
        case VT_DOUBLE: {
          AsmJit::Label grL = _.newLabel();
          AsmJit::Label exitL = _.newLabel();
          GPVar res(_.newGP());
          _.comisd(popOperand<XMMVar>(), popOperand<XMMVar>());
          _.jg(grL);

          _.mov(res, Imm(0));
          _.jmp(exitL);

          _.bind(grL);

          _.mov(res, Imm(1));

          _.bind(exitL);

          pushOperand(res);
          break;
        }
        default:
          throw runtime_error("wrong type operand for '>'");
      }
    }

    inline void genGe() {
      switch (binaryOpLConversion()) {
        case VT_INT: {
          AsmJit::Label geL = _.newLabel();
          AsmJit::Label exitL = _.newLabel();
          GPVar rOp = popOperand<GPVar>();
          _.cmp(popOperand<GPVar>(), rOp);
          _.jge(geL);

          _.mov(rOp, Imm(0));
          _.jmp(exitL);

          _.bind(geL);

          _.mov(rOp, Imm(1));

          _.bind(exitL);

          pushOperand(rOp);
          break;
        }
        case VT_DOUBLE: {
          AsmJit::Label geL = _.newLabel();
          AsmJit::Label exitL = _.newLabel();
          GPVar res(_.newGP());
          _.comisd(popOperand<XMMVar>(), popOperand<XMMVar>());
          _.jge(geL);

          _.mov(res, Imm(0));
          _.jmp(exitL);

          _.bind(geL);

          _.mov(res, Imm(1));

          _.bind(exitL);

          pushOperand(res);
          break;
        }
        default:
          throw runtime_error("wrong type operand for '>='");
      }
    }

    inline void genLe() {
      switch (binaryOpLConversion()) {
        case VT_INT: {
          AsmJit::Label leL = _.newLabel();
          AsmJit::Label exitL = _.newLabel();
          GPVar rOp = popOperand<GPVar>();
          _.cmp(popOperand<GPVar>(), rOp);
          _.jle(leL);

          _.mov(rOp, Imm(0));
          _.jmp(exitL);

          _.bind(leL);

          _.mov(rOp, Imm(1));

          _.bind(exitL);

          pushOperand(rOp);
          break;
        }
        case VT_DOUBLE: {
          AsmJit::Label leL = _.newLabel();
          AsmJit::Label exitL = _.newLabel();
          GPVar res(_.newGP());
          _.comisd(popOperand<XMMVar>(), popOperand<XMMVar>());
          _.jle(leL);

          _.mov(res, Imm(0));
          _.jmp(exitL);

          _.bind(leL);

          _.mov(res, Imm(1));

          _.bind(exitL);

          pushOperand(res);
          break;
        }
        default:
          throw runtime_error("wrong type operand for '<='");
      }
    }

    inline void genAnd() {
      switch (binaryOpLConversion()) {
        case VT_INT: {
          AsmJit::Label falseL = _.newLabel();
          AsmJit::Label exitL = _.newLabel();
          GPVar rOp = popOperand<GPVar>();
          GPVar lOp = popOperand<GPVar>();

          _.cmp(lOp, 0);
          _.je(falseL);

          _.cmp(rOp, 0);
          _.je(falseL);

          _.jmp(exitL);

          _.bind(falseL);
          _.mov(lOp, 0);

          _.bind(exitL);

          pushOperand(lOp);
          break;
        }
        case VT_DOUBLE: {
          AsmJit::Label trueL = _.newLabel();
          AsmJit::Label falseL = _.newLabel();
          AsmJit::Label exitL = _.newLabel();
          GPVar rOp(_.newGP());
          _.cvtsd2si(rOp, popOperand<XMMVar>());
          GPVar lOp(_.newGP());
          _.cvtsd2si(lOp, popOperand<XMMVar>());

          _.cmp(lOp, 0);
          _.je(falseL);

          _.cmp(rOp, 0);
          _.je(falseL);

          _.jmp(exitL);

          _.bind(falseL);
          _.mov(lOp, 0);

          _.bind(exitL);

          pushOperand(lOp);
          break;
        }
        default:
          throw runtime_error("wrong type operand for '&&'");
      }
    }

    inline void genOr() {
      switch (binaryOpLConversion()) {
        case VT_INT: {
          AsmJit::Label trueL = _.newLabel();
          AsmJit::Label falseL = _.newLabel();
          AsmJit::Label exitL = _.newLabel();
          GPVar rOp = popOperand<GPVar>();
          GPVar lOp = popOperand<GPVar>();

          _.cmp(lOp, 0);
          _.jne(trueL);

          _.cmp(rOp, 0);
          _.jne(trueL);

          _.mov(lOp, 0);
          _.jmp(exitL);

          _.bind(trueL);
          _.mov(lOp, 1);

          _.bind(exitL);

          pushOperand(lOp);
          break;
        }
        case VT_DOUBLE: {
          AsmJit::Label trueL = _.newLabel();
          AsmJit::Label falseL = _.newLabel();
          AsmJit::Label exitL = _.newLabel();
          GPVar rOp(_.newGP());
          _.cvtsd2si(rOp, popOperand<XMMVar>());
          GPVar lOp(_.newGP());
          _.cvtsd2si(lOp, popOperand<XMMVar>());

          _.cmp(lOp, 0);
          _.jne(trueL);

          _.cmp(rOp, 0);
          _.jne(trueL);

          _.mov(lOp, 0);
          _.jmp(exitL);

          _.bind(trueL);
          _.mov(lOp, 1);

          _.bind(exitL);

          pushOperand(lOp);
          break;
        }
        default:
          throw runtime_error("wrong type operand for '||'");
      }
    }

    inline void genAOr() {
      switch (binaryOpConversion()) {
        case VT_INT: {
          GPVar rOp = popOperand<GPVar>();
          _.or_(rOp, popOperand<GPVar>());
          pushOperand(rOp);
          break;
        }
        default:
          throw runtime_error("wrong type operand for '|'");
      }
    }

    inline void genAAnd() {
      switch (binaryOpConversion()) {
        case VT_INT: {
          GPVar rOp = popOperand<GPVar>();
          _.and_(rOp, popOperand<GPVar>());
          pushOperand(rOp);
          break;
        }
        default:
          throw runtime_error("wrong type operand for '&'");
      }
    }

    inline void genAXor() {
      switch (binaryOpConversion()) {
        case VT_INT: {
          GPVar rOp = popOperand<GPVar>();
          _.xor_(rOp, popOperand<GPVar>());
          pushOperand(rOp);
          break;
        }
        default:
          throw runtime_error("wrong type operand for '^'");
      }
    }

  };

}

#endif //__MachineCodeGenerator_H_
