//
// Created by Vadim Lomshakov on 13/12/13.
// Copyright (c) 2013 spbau. All rights reserved.
//

#include <inttypes.h>
#include "MachineCodeGenerator.h"

namespace mathvm {

#define log(A,...) //if (_.getLogger()) { _.comment(A,##__VA_ARGS__); }


//[Exprs] =============================================================================//
  void MachineCodeGenerator::visitDoubleLiteralNode(DoubleLiteralNode *node) {
    log("double load start")
    if (_assignmentCnt) {
      XMMVar op(_.newXMM(VARIABLE_TYPE_XMM_1D));
      double d = node->literal();
      int64_t i = *((int64_t*)&d);

      // inline value to instr stream
      GPVar tmp(_.newGP());
      _.mov(tmp, Imm(i));
      _.movq(op, tmp);
      _.unuse(tmp);

      pushType(VT_DOUBLE);
      pushOperand(op);
    }
    log("double end")
  }

  void MachineCodeGenerator::visitStringLiteralNode(StringLiteralNode *node) {
    log("str load start")
    if (_assignmentCnt) {
      char const* sDesc = _code->makeOrGetStringConstant(node->literal());

      GPVar op(_.newGP());
      _.mov(op, Imm((sysint_t) sDesc));

      pushType(VT_STRING);
      pushOperand(op);
    }
    log("str end")
  }

  void MachineCodeGenerator::visitIntLiteralNode(IntLiteralNode *node) {
    log("int load start")
    if (_assignmentCnt) {
      GPVar op(_.newGP());
      _.mov(op, Imm(node->literal()));

      pushType(VT_INT);
      pushOperand(op);
    }
    log("int end")
  }

  void MachineCodeGenerator::visitLoadNode(LoadNode *node) {
    //TOD0 if var is local, generate simple code
    log("load start")

    if (_assignmentCnt) {
      AstVar const* astVar = node->var();
      VarType type = astVar->type();

      // get our var
      BaseVar var;
      if (type != VT_DOUBLE) {
        GPVar _var(_.newGP());
        _.mov(_var, getVarFromStack(astVar)); //qword_ptr(getVarAddressOnStack(astVar)));
        var = _var;
      } else {
        XMMVar _var(_.newXMM(VARIABLE_TYPE_XMM_1D));
        _.movq(_var, getVarFromStack(astVar));
        var = _var;
      }

      pushType(type);
      pushOperand(var);
    }

    log("load end")
  }

  void MachineCodeGenerator::visitBinaryOpNode(BinaryOpNode *node) {
    AstBaseVisitor::visitBinaryOpNode(node);

    if (!_assignmentCnt) return;

    switch (node->kind()) {
      case tADD:
        genAdd();
        break;
      case tSUB:
        genSub();
        break;
      case tMUL:
        genMul();
        break;
      case tDIV:
        genDiv();
        break;
      case tMOD:
        genMod();
        break;
      case tAOR:
        genAOr();
        break;
      case tAAND:
        genAAnd();
        break;
      case tAXOR:
        genAXor();
        break;
      case tEQ:
        genEq();
        break;
      case tNEQ:
        genNeq();
        break;
      case tLT:
        genLt();
        break;
      case tGT:
        genGt();
        break;
      case tGE:
        genGe();
        break;
      case tLE:
        genLe();
        break;
      case tAND:
        genAnd();
        break;
      case tOR:
        genOr();
        break;
      default:
        throw new logic_error("wrong operator kind " + string(tokenStr(node->kind())));
    }
  }

  void MachineCodeGenerator::visitUnaryOpNode(UnaryOpNode *node) {
    AstBaseVisitor::visitUnaryOpNode(node);

    if (!_assignmentCnt) return;

    switch (node->kind()) {
      case tSUB:
        genNeg();
        break;
      case tNOT:
        genNot();
        break;
      case tADD:
        break;
      default:
        throw new logic_error("wrong operator kind " + string(tokenStr(node->kind())));
    }
  }

//[Stmts] =============================================================================//
  void MachineCodeGenerator::visitPrintNode(PrintNode *node) {
    static char const * fFormat = "%g";
    static char const * dFormat = "%"PRId64;
    log("start print node")
    ++_assignmentCnt;

    GPVar address(_.newGP());
    _.mov(address, Imm((sysint_t)_printfSym));

    for (uint32_t i = 0; i < node->operands(); i++) {
      node->operandAt(i)->visit(this);

      GPVar formatStr(_.newGP());
      GPVar ret(_.newGP());
      FunctionBuilderX dfn;
      dfn.setReturnValue<int>();

      // make format string arg
      VarType typeOp = popType();

      if (typeOp == VT_INT) { _.mov(formatStr, Imm((sysint_t) dFormat)); dfn.addArgument<char const*>(); dfn.addArgument<int64_t>(); }
      else if (typeOp == VT_DOUBLE) { _.mov(formatStr, Imm((sysint_t) fFormat)); dfn.addArgument<char const*>(); dfn.addArgument<double>(); }
      else if (typeOp == VT_STRING) { dfn.addArgument<char const*>(); }
      else throw runtime_error("operand of print instr has invalid type");

      // call printf
      ECall* ctx = _.call(address);
      ctx->setPrototype(_ccnv, dfn);

      // pass args
      uint32_t numArg = 0;
      if (typeOp != VT_STRING) {
        ctx->setArgument(numArg++, formatStr);
      }
      ctx->setArgument(numArg, popOperand<BaseVar>());
      ctx->setReturn(ret);
    }

    --_assignmentCnt;
    log("end print node")
  }

  void MachineCodeGenerator::visitStoreNode(StoreNode *node) {  /*qword_ptr(getVarAddressOnStack(astVar))*/
    log("store start")
    ++_assignmentCnt;
    AstBaseVisitor::visitStoreNode(node);

    AstVar const* astVar = node->var();
    VarType leftType = astVar->type();
    VarType rightType = popType();

    BaseVar retVal = assignmentConversion(leftType, rightType);

    // set our var
    if (retVal.isXMMVar()) {
      Mem var = getVarFromStack(astVar);
      XMMVar val = static_cast<XMMVar&>(retVal);
      if (node->op() == tINCRSET)
        _.addsd(val, var);
      if (node->op() == tDECRSET) {
        pushType(VT_DOUBLE);
        pushOperand(val);
        genNeg();
        popType();
        val = popOperand<XMMVar>();
        _.addsd(val, var);
      }
      _.movq(var, val);
    }
    if (retVal.isGPVar()) {
      Mem var = getVarFromStack(astVar);
      GPVar val = static_cast<GPVar&>(retVal);
      if (node->op() == tINCRSET)
        _.add(val, var);
      if (node->op() == tDECRSET) {
        _.neg(val);
        _.add(val, var);
      }
      _.mov(var, val);
    }

    --_assignmentCnt;
    log("store end")
  }

//[ControlFlow Stmts] =============================================================================//
  void MachineCodeGenerator::visitIfNode(IfNode *node) {
    ++_assignmentCnt;
    node->ifExpr()->visit(this);
    --_assignmentCnt;

    if (popType() != VT_INT) throw runtime_error("unexpected type of condition expr into if");

    AsmJit::Label elseBranchL = _.newLabel();
    AsmJit::Label exitL = _.newLabel();
    _.cmp(popOperand<GPVar>(), 0);
    _.je(elseBranchL);

    node->thenBlock()->visit(this);

    _.jmp(exitL);
    _.bind(elseBranchL);

    if (node->elseBlock() != 0)
      node->elseBlock()->visit(this);

    _.bind(exitL);
  }

  void MachineCodeGenerator::visitForNode(ForNode *node) {
    BinaryOpNode* range = node->inExpr()->asBinaryOpNode();
    AstNode* begin = range->left();
    AstNode* end = range->right();

    AsmJit::Label condL = _.newLabel();
    AsmJit::Label bodyL = _.newLabel();

    ++_assignmentCnt;
    begin->visit(this);
    --_assignmentCnt;
    if (popType() != VT_INT) throw runtime_error("unexpected type of begin expr into while");
    GPVar initialVal = popOperand<GPVar>();

    Mem loopVar = getVarFromStack(node->var());
    _.mov(loopVar, initialVal);


    _.jmp(condL);
    _.bind(bodyL);

    node->body()->visit(this);

    _.add(loopVar, 1);
    _.bind(condL);

    ++_assignmentCnt;
    end->visit(this);
    --_assignmentCnt;
    if (popType() != VT_INT) throw runtime_error("unexpected type of begin expr into while");
    GPVar endVal = popOperand<GPVar>();

    _.cmp(loopVar, endVal);
    _.jng(bodyL);
  }

  void MachineCodeGenerator::visitWhileNode(WhileNode *node) {
    AsmJit::Label condL = _.newLabel();
    AsmJit::Label bodyL = _.newLabel();

    _.jmp(condL);
    _.bind(bodyL);

    node->loopBlock()->visit(this);

    _.bind(condL);

    ++_assignmentCnt;
    node->whileExpr()->visit(this);
    --_assignmentCnt;

    if (popType() != VT_INT) throw runtime_error("unexpected type of condition expr into while");


    _.cmp(popOperand<GPVar>(), 0);
    _.jne(bodyL);
  }

//[Function&Scopes Stms] =============================================================================//
  void MachineCodeGenerator::visitCallNode(CallNode *node) {
    log("call node start")
    ++_assignmentCnt;
    AstBaseVisitor::visitCallNode(node);
    --_assignmentCnt;

    MachCodeFunc* func = _code->functionByName(node->name());
    void** code = func->machcode();

    GPVar addressMem(_.newGP());
    _.mov(addressMem, Imm((sysint_t) code));

    ECall* ctx = _.call(sysint_ptr(addressMem));
    FunctionBuilderX defn; buildFunctionPrototypeBy(func, defn);
    ctx->setPrototype(_ccnv, defn);

    for (uint32_t i = 0; i != func->parametersNumber(); ++i) {
      VarType leftType = func->parameterType(i);
      VarType rightType = popType();

      ctx->setArgument(i, assignmentConversion(leftType, rightType));
    }

    VarType retType = func->returnType();
    if (retType != VT_VOID) {
      BaseVar retVal;
      if (retType != VT_DOUBLE) {
        GPVar _retVal(_.newGP());
        ctx->setReturn(_retVal);

        retVal = _retVal;
      } else {
        XMMVar _retVal(_.newXMM(VARIABLE_TYPE_XMM_1D));
        ctx->setReturn(_retVal);

        retVal = _retVal;
      }

      if (_assignmentCnt) {
        pushType(retType);
        _stackOperand.push(retVal);
      }
    } else
      assert(_assignmentCnt == 0);


    log("call node end")
  }

  void MachineCodeGenerator::visitReturnNode(ReturnNode *node) {
    log("return node start")
    if (node->returnExpr() != 0) {
      ++_assignmentCnt;
      node->returnExpr()->visit(this);
      --_assignmentCnt;

      VarType leftType = _currGenFunc->returnType();
      VarType rightType = popType();

      BaseVar retVal = assignmentConversion(leftType, rightType);

      pushOperand(retVal);
      pushType(leftType);
    }
    log("return node end")
  }

  void MachineCodeGenerator::visitBlockNode(BlockNode *node) {
    log("block node start ")
    AstBaseVisitor::visitBlockNode(node);
    log("block end")
  }

  void MachineCodeGenerator::visitNativeCallNode(NativeCallNode *node) {
    log("native call start")
    void* sym = initLIBCSym(node->nativeName());
    GPVar address(_.newGP());
    _.mov(address, Imm((sysint_t)sym));

    ECall* ctx = _.call(address);
    FunctionBuilderX defn; buildFunctionPrototypeBy(_currGenFunc, defn);
    ctx->setPrototype(_ccnv, defn);

    assert(_stackOperand.size() >= _currGenFunc->parametersNumber());

    for (uint32_t i = 0; i != _currGenFunc->parametersNumber(); ++i) {
      // access directly, not use virtual stack
      ctx->setArgument(i, _.argGP(i));
    }

    VarType retType = _currGenFunc->returnType();
    if (retType != VT_VOID) {
      if (retType != VT_DOUBLE) {
        GPVar retVal(_.newGP());
        ctx->setReturn(retVal);

        // directly return value, don't use return stmt
        _.ret(retVal);
      } else {
        XMMVar retVal(_.newXMM(VARIABLE_TYPE_XMM_1D));
        ctx->setReturn(retVal);

        _.ret(retVal);
      }
    }
    log("native call end")
  }

  void MachineCodeGenerator::visitFunctionNode(FunctionNode *node) {
    _currGenFunc = _code->functionByName(node->name());

    log("Function %s", node->name().c_str())
    FunctionBuilderX defn; buildFunctionPrototypeBy(_currGenFunc, defn);
    _.newFunction(_ccnv, defn);

    // magick work around for bug in AsmJit
    divider = _.newGP();
    _.setPriority(divider, 100);
    vprolog();//      prolog();

    // make body
    node->body()->visit(this);


    vepilog();//    epilog();
    appendReturn();
    _.endFunction();

    // compile this function
    _currGenFunc->setCode(_.make());

    _.clear();
    _currGenFunc = 0;
    _assignmentCnt = 0;
    _typeStack = stack<VarType>();
    _stackOperand = stack<BaseVar>();
  }

  void MachineCodeGenerator::appendReturn() {
    if (_currGenFunc->returnType() != VT_VOID) {
      log("append return Insn")

      if (popType() != VT_DOUBLE)
        _.ret(popOperand<GPVar>());
      else
        _.ret(popOperand<XMMVar>());
    }
  }



//[Other] =============================================================================//

  void MachineCodeGenerator::generate() {
//    FileLogger logger(stdout);
//    _.setLogger(&logger);

    vector<FunctionNode*> funcsForGen;
    InitPass preprocess(_top, _code, _addressByVar, funcsForGen);
    preprocess.doInit();

    for (size_t i = 0; i != funcsForGen.size(); ++i) {
      visitFunctionNode(funcsForGen[i]);
    }

  }

  void MachineCodeGenerator::buildFunctionPrototypeBy(MachCodeFunc *func, FunctionBuilderX& defn) {
    for (uint32_t i = 0; i != func->parametersNumber(); ++i) {
      switch (func->parameterType(i)) {
        case VT_INT:
          defn.addArgument<int64_t>();
          break;
        case VT_DOUBLE:
          defn.addArgument<double>();
          break;
        case VT_STRING:
          defn.addArgument<char const*>();
          break;
        default:
          throw runtime_error("function " + func->name() + " has wrong type of arg " + func->parameterName(i));
      }
    }

    switch (func->returnType()) {
      case VT_INT:
        defn.setReturnValue<int64_t>();
        break;
      case VT_DOUBLE:
        defn.setReturnValue<double>();
        break;
      case VT_STRING:
        defn.setReturnValue<char const*>();
        break;
      case VT_VOID:
        defn.setReturnValue<void>();
        break;
      default:
        throw runtime_error("function " + func->name() + " has wrong return type");
    }
  }

#define GET_DISPLAY_DISPL(offset)\
  ((offset) * VCtx::DISPLAY_REC_SIZE)
#define GET_VAR_DISPL(offset)\
  ((offset) * MachCodeFunc::VAR_SIZE)

  void MachineCodeGenerator::vprolog() {
    log("Virt prolog")

    VCtx& vctx = _code->vcontext();

    GPVar stackBase(_.newGP());
    _.mov(stackBase, Imm((sysint_t)((void*)vctx.stack)));

    GPVar vbpMem(_.newGP());
    GPVar vbp(_.newGP());
    _.mov(vbpMem, Imm((sysint_t)(void*)&vctx.BP));
    _.mov(vbp, qword_ptr(vbpMem));

    GPVar vspMem(_.newGP());
    GPVar vsp(_.newGP());
    _.mov(vspMem, Imm((sysint_t)(void*)&vctx.SP));
    _.mov(vsp, qword_ptr(vspMem));

    // save virtual value BP reg
    GPVar offset(_.newGP());
    _.mov(offset, stackBase);
    _.add(offset, vsp);
    _.add(offset, GET_VAR_DISPL(VCtx::VBP_OFFSET));
    _.mov(qword_ptr(offset), vbp);

    // move vsp to vbp
    _.mov(vbp, vsp);
    _.mov(qword_ptr(vbpMem), vbp);


    // allocate memory in stack: add vsp, size_stack
    if (_currGenFunc->getVStackSize() > 0) {
      _.add(vsp, Imm(_currGenFunc->getVStackSize()));
      _.mov(qword_ptr(vspMem), vsp);
    }

    uint32_t argsStartOffset = VCtx::ARGS_BEGIN_OFFSET;
    // load args to virtual stack
    for (uint32_t i = 0; i != _currGenFunc->parametersNumber(); ++i) {
      GPVar arg;
      XMMVar argD;

      if (_currGenFunc->parameterType(i) != VT_DOUBLE) {
        arg = _.argGP(0);
      } else {
        argD = _.argXMM(0);
      }

      // stackBase + vbp + offset * idx_arg
      GPVar offsetArg(_.newGP());
      _.mov(offsetArg, stackBase);
      _.add(offsetArg, vbp);
      _.add(offsetArg, GET_VAR_DISPL(i + argsStartOffset));

      if (_currGenFunc->parameterType(i) != VT_DOUBLE)
        _.mov(qword_ptr(offsetArg), arg);
      else
        _.movq(qword_ptr(offsetArg), argD);

      _.unuse(offsetArg);
      _.unuse(arg);
      _.unuse(argD);
    }

    GPVar dispMem(_.newGP());
    _.mov(dispMem, Imm((sysint_t)(void*)vctx.display));

    // save prev access link into display
    GPVar disp(_.newGP());
    _.mov(offset, stackBase);
    _.add(offset, vbp);
    _.add(offset, GET_VAR_DISPL(VCtx::PREV_LINK_ACCESS_OFFSET));
    _.mov(disp, qword_ptr(dispMem, GET_DISPLAY_DISPL(_currGenFunc->getDepth())));
    _.mov(qword_ptr(offset), disp);


    // store current access link
    _.mov(qword_ptr(dispMem, GET_DISPLAY_DISPL(_currGenFunc->getDepth())), vbp);


    _.unuse(offset);
    _.unuse(vbp); _.unuse(vbpMem);
    _.unuse(vsp); _.unuse(vspMem);
    _.unuse(stackBase);
    _.unuse(dispMem); _.unuse(dispMem);

    log("Our Body")
  }

  void MachineCodeGenerator::vepilog() {
    log("Virt epilog")
    VCtx& vctx = _code->vcontext();


    GPVar stackBase(_.newGP());
    _.mov(stackBase, Imm((sysint_t)((void*)vctx.stack)));
    GPVar vbpMem(_.newGP());
    _.mov(vbpMem, Imm((sysint_t)(void*)&vctx.BP));


    GPVar dispMem(_.newGP());
    _.mov(dispMem, Imm((sysint_t)(void*)vctx.display));

    // restore prev access link into display
    GPVar tmp(_.newGP());
    GPVar offset(_.newGP());
    _.mov(offset, stackBase);
    _.add(offset, qword_ptr(vbpMem));
    _.add(offset, GET_VAR_DISPL(VCtx::PREV_LINK_ACCESS_OFFSET));
    _.mov(tmp, qword_ptr(offset));
    _.mov(qword_ptr(dispMem, GET_DISPLAY_DISPL(_currGenFunc->getDepth())), tmp);

    // restore vbp
    _.mov(offset, stackBase);
    _.add(offset, qword_ptr(vbpMem));
    _.add(offset, GET_VAR_DISPL(VCtx::VBP_OFFSET));
    _.mov(tmp, qword_ptr(offset));
    _.mov(qword_ptr(vbpMem), tmp);


    GPVar vspMem(_.newGP());
    GPVar vsp(_.newGP());
    _.mov(vspMem, Imm((sysint_t)(void*)&vctx.SP));
    _.mov(vsp, qword_ptr(vspMem));

    // restore vsp
    _.sub(vsp, Imm(_currGenFunc->getVStackSize()));
    _.mov(qword_ptr(vspMem), vsp);


    _.unuse(offset);
    _.unuse(tmp);
    _.unuse(vbpMem);
    _.unuse(vsp); _.unuse(vspMem);
    _.unuse(dispMem);
    _.unuse(stackBase);
  }


  Mem MachineCodeGenerator::getVarFromStack(AstVar const *astVar) {
    log("getVarFromStack start")
    VCtx& vctx = _code->vcontext();
    VarId key = make_pair(astVar->name(), getScopeUID(astVar->owner()));

    assert(_addressByVar.count(key) == 1);

    VarAddress address = _addressByVar[key];
    int32_t offsetOnStk = address.first;
    int32_t idxDisplay = address.second;

    GPVar dispMem(_.newGP());
    GPVar vbp(_.newGP());
    GPVar stackBase(_.newGP());
    GPVar offset(_.newGP());

    // get display base
    _.mov(dispMem, Imm((sysint_t)(void*)vctx.display));

    // get access link into display
    _.mov(vbp, qword_ptr(dispMem, GET_DISPLAY_DISPL(idxDisplay)));

    // get stack base
    _.mov(stackBase, Imm((sysint_t)(void*)vctx.stack));

    // get address of our var
    _.mov(offset, stackBase);
    _.add(offset, vbp);
    _.add(offset, GET_VAR_DISPL(offsetOnStk));
    Mem varMem = qword_ptr(offset);

    _.unuse(vbp);
    _.unuse(stackBase);
    _.unuse(dispMem);
    log("getVarFromStack end")
    return varMem;
  }



  MachineCodeGenerator::~MachineCodeGenerator() {

  }

// DEBUG =====================================================================================//
  void MachineCodeGenerator::prolog() {
    VCtx& vctx = _code->vcontext();
    log("Prog prolog")

    GPVar arg0(_.newGP());
    GPVar arg1(_.newGP());

    _.mov(arg0, Imm((sysint_t)(void*) &(_code->vcontext())));
    _.mov(arg1, Imm((sysint_t)(void*) _currGenFunc));


    GPVar address(_.newGP());
    _.mov(address, Imm((sysint_t) (void*)pprolog));

    ECall* ctx = _.call(address);
    ctx->setPrototype(_ccnv, FunctionBuilder2<void, VCtx*, MachCodeFunc*>());
    ctx->setArgument(0, arg0);
    ctx->setArgument(1, arg1);




    GPVar stackBase(_.newGP());
    _.mov(stackBase, Imm((sysint_t)((void*)vctx.stack)));

    GPVar vbpMem(_.newGP());
    GPVar vbp(_.newGP());
    _.mov(vbpMem, Imm((sysint_t)(void*)&vctx.BP));
    _.mov(vbp, qword_ptr(vbpMem));


    uint32_t argsStartOffset = VCtx::ARGS_BEGIN_OFFSET;
    // load args to virtual stack
    for (uint32_t i = 0; i != _currGenFunc->parametersNumber(); ++i) {
      GPVar arg;
      XMMVar argD;

      if (_currGenFunc->parameterType(i) != VT_DOUBLE) {
        arg = _.argGP(0);
      } else {
        argD = _.argXMM(0);
      }

      // stackBase + vbp + offset * idx_arg
      GPVar offsetArg(_.newGP());
      _.mov(offsetArg, stackBase);
      _.add(offsetArg, vbp);
      _.add(offsetArg, GET_VAR_DISPL(i + argsStartOffset));

      if (_currGenFunc->parameterType(i) != VT_DOUBLE)
        _.mov(qword_ptr(offsetArg), arg);
      else
        _.movq(qword_ptr(offsetArg), argD);

      _.unuse(offsetArg);
      _.unuse(arg);
      _.unuse(argD);
    }

    log("Our Body")
  }

  void MachineCodeGenerator::epilog() {
   log("Prog epilog")

    GPVar arg0(_.newGP());
    GPVar arg1(_.newGP());
    _.mov(arg0, Imm((sysint_t)(void*) &(_code->vcontext())));
    _.mov(arg1, Imm((sysint_t)(void*) _currGenFunc));


    GPVar address(_.newGP());
    _.mov(address, Imm((sysint_t) (void*)pepilog));

    ECall* ctx = _.call(address);
    ctx->setPrototype(_ccnv, FunctionBuilder2<void, VCtx*, MachCodeFunc*>());
    ctx->setArgument(0, arg0);
    ctx->setArgument(1, arg1);
  }

  GPVar MachineCodeGenerator::getVarAddressOnStack(AstVar const *astVar) {
    log("getVarAddressOnStack start")

    VarId key = make_pair(astVar->name(), getScopeUID(astVar->owner()));

    assert(_addressByVar.count(key) == 1);

    VarAddress varAddress = _addressByVar[key];
    int32_t offsetOnStk = varAddress.first;
    int32_t idxDisplay = varAddress.second;


    GPVar arg0(_.newGP());
    GPVar arg1(_.newGP());
    GPVar arg2(_.newGP());
    GPVar retVal(_.newGP());
    _.mov(arg0, Imm(offsetOnStk));
    _.mov(arg1, Imm(idxDisplay));
    _.mov(arg2, Imm((sysint_t)(void*) &(_code->vcontext())));


    GPVar address(_.newGP());
    _.mov(address, Imm((sysint_t)(void*) getVarAddress));

    ECall* ctx = _.call(address);
    ctx->setPrototype(_ccnv, FunctionBuilder3<void *, int32_t , int32_t, VCtx*>());
    ctx->setArgument(0, arg0);
    ctx->setArgument(1, arg1);
    ctx->setArgument(2, arg2);
    ctx->setReturn(retVal);

    return retVal;
  }

#undef GET_DISPLAY_DISPL
#undef GET_VAR_DISPL
#undef log
}