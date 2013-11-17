//
// Created by Vadim Lomshakov on 10/26/13.
// Copyright (c) 2013 spbau. All rights reserved.
//


#include <math.h>
#include "InterpreterImpl.h"

namespace mathvm {

  void InterpreterImpl::setInitialState() {
    assert(functionByName(AstFunction::top_name) != 0);

    callFunction(0);
  }

  void InterpreterImpl::bindTopLevelVars(vector<Var *> &vars) {

  }

  void InterpreterImpl::unbindTopLevelVars(vector<Var *> &vars) {

  }

  void InterpreterImpl::callFunction(uint16_t fid) {
//    debugInfo();
    BytecodeFunction* foo = static_cast<BytecodeFunction*>(functionById(fid));

    if (_mapFrames.size() <= fid) {
      _mapFrames.resize(fid + 1);
    }

    StackFrame calleeFrame(foo->localsNumber() + foo->parametersNumber());

    if (foo->parametersNumber() != 0) {
      assert(foo->name() != AstFunction::top_name);

      uint16_t callerFid = _callStack.top().fid;
      StackFrame& callerFrame = _mapFrames[callerFid].back();

      assert(callerFrame.stack.size() >= foo->parametersNumber());
      // moves args
      for (int i = foo->parametersNumber() - 1; i >= 0 ; --i) {
        calleeFrame.locals[i] = callerFrame.stack.pop();
      }
    }

    _mapFrames[fid].push_back(calleeFrame);


    _callStack.push(FunctionDescriptor(fid));
    _currentCode = foo->bytecode();
  }

  void InterpreterImpl::returnFromFunction() {
    uint16_t fid = _callStack.top().fid;
    StackFrame frame = _mapFrames[fid].back();
    _callStack.pop();
    _mapFrames[fid].pop_back();

    TranslatedFunction* foo = functionById(fid);
    assert((frame.stack.empty() && (functionById(fid)->returnType() == VT_VOID))
        || ((frame.stack.size() == 1) && (functionById(fid)->returnType() != VT_VOID)));

    if (foo->returnType() != VT_VOID) {
      assert(foo->name() != AstFunction::top_name);
      assert(!_mapFrames[_callStack.top().fid].empty());

      StackFrame& callerFrame = _mapFrames[_callStack.top().fid].back();
      callerFrame.stack.push(frame.stack.pop());
    }

    _currentCode = static_cast<BytecodeFunction*>(functionById(_callStack.top().fid))->bytecode();
  }

  void InterpreterImpl::executeI2D() {
    currentStack().pushDoubleValue(currentStack().popIntValue());
  }

  void InterpreterImpl::executeLOADDVAR() {
    currentStack().push(currentLocals()[getNextUInt16()]);
  }

  void InterpreterImpl::executeS2I() {
    assert("method is not implement" == 0);
  }

  void InterpreterImpl::executeRETURN() {
    returnFromFunction();
  }

  void InterpreterImpl::executeJA() {
    jump(getNextInt16());
  }

  void InterpreterImpl::executeLOADSVAR3() {
    assert("method is not implement" == 0);
  }

  void InterpreterImpl::executeLOADSVAR2() {
    assert("method is not implement" == 0);
  }

  void InterpreterImpl::executeLOADSVAR1() {
    assert("method is not implement" == 0);
  }

  void InterpreterImpl::executeLOADSVAR0() {
    assert("method is not implement" == 0);
  }

  void InterpreterImpl::executeDLOAD() {
    currentFrameStack().stack.pushDoubleValue(getNextDouble());
  }

  void InterpreterImpl::executeCALLNATIVE() {
    assert("method is not implement" == 0);
  }

  void InterpreterImpl::executeINVALID() {
    assert("method is not implement" == 0);
  }

  void InterpreterImpl::executePOP() {
    assert(!currentStack().empty());
    currentStack().pop_back();
  }

  void InterpreterImpl::executeLOADCTXDVAR() {
    assert("method is not implement" == 0);
  }

  void InterpreterImpl::executeSTORESVAR0() {
    assert("method is not implement" == 0);
  }

  void InterpreterImpl::executeLOADIVAR() {
    currentStack().push(currentLocals()[getNextUInt16()]);
  }

  void InterpreterImpl::executeIPRINT() {
    assert(!currentStack().empty());
    out << currentStack().popIntValue();
  }

  void InterpreterImpl::executeDLOADM1() {
    assert("method is not implement" == 0);
  }

  void InterpreterImpl::executeIFICMPGE() {
    currentStack().popIntValue() >= currentStack().popIntValue() ? jump(getNextInt16()) : (void)getNextInt16();
  }

  void InterpreterImpl::executeSTORESVAR() {
    assert(!currentStack().empty());
    currentLocals()[getNextUInt16()] = currentStack().pop();
  }

  void InterpreterImpl::executeSLOAD0() {
    assert("method is not implement" == 0);
  }

  void InterpreterImpl::executeILOAD() {
    currentStack().pushIntValue(getNextInt64());
  }

  void InterpreterImpl::executeSTOREDVAR3() {
    assert("method is not implement" == 0);
  }

  void InterpreterImpl::executeSTOREDVAR2() {
    assert("method is not implement" == 0);
  }

  void InterpreterImpl::executeSTOREDVAR1() {
    assert("method is not implement" == 0);
  }

  void InterpreterImpl::executeSTOREDVAR0() {
    assert("method is not implement" == 0);
  }

  void InterpreterImpl::executeDMUL() {
    assert(currentStack().size() >= 2);
    currentStack().pushDoubleValue(currentStack().popDoubleValue() * currentStack().popDoubleValue());
  }

  void InterpreterImpl::executeIAOR() {
    assert(currentStack().size() >= 2);
    currentStack().pushIntValue(currentStack().popIntValue() | currentStack().popIntValue());
  }

  void InterpreterImpl::executeDADD() {
    assert(currentStack().size() >= 2);
    currentStack().pushDoubleValue(currentStack().popDoubleValue() + currentStack().popDoubleValue());
  }

  void InterpreterImpl::executeSTORECTXSVAR() {
    assert("method is not implement" == 0);
  }

  void InterpreterImpl::executeDPRINT() {
    assert(!currentStack().empty());
    out << currentStack().popDoubleValue();
  }

  void InterpreterImpl::executeSTOREIVAR3() {
    assert("method is not implement" == 0);
  }

  void InterpreterImpl::executeSTOREIVAR2() {
    assert("method is not implement" == 0);
  }

  void InterpreterImpl::executeSTOREIVAR1() {
    assert("method is not implement" == 0);
  }

  void InterpreterImpl::executeSTOREIVAR0() {
    assert("method is not implement" == 0);
  }

  void InterpreterImpl::executeIFICMPLE() {
    currentStack().popIntValue() <= currentStack().popIntValue() ? jump(getNextInt16()) : (void)getNextInt16();;
  }

  void InterpreterImpl::executeDLOAD1() {
    currentStack().pushDoubleValue(1.0);
  }

  void InterpreterImpl::executeD2I() {
    assert("method is not implement" == 0);
  }

  void InterpreterImpl::executeDNEG() {
    assert(!currentStack().empty());
    currentStack().pushDoubleValue(-currentStack().popDoubleValue());
  }

  void InterpreterImpl::executeIADD() {
    assert(currentStack().size() >= 2);
    currentStack().pushIntValue(currentStack().popIntValue() + currentStack().popIntValue());
  }

  void InterpreterImpl::executeLOADSVAR() {
    currentStack().push(currentLocals()[getNextUInt16()]);
  }

  void InterpreterImpl::executeIMUL() {
    assert(currentStack().size() >= 2);
    currentStack().pushIntValue(currentStack().popIntValue() * currentStack().popIntValue());
  }

  void InterpreterImpl::executeSLOAD() {
    currentStack().pushStringValue(getNextUInt16());
  }

  void InterpreterImpl::executeILOADM1() {
    currentStack().pushIntValue(-1);
  }

  void InterpreterImpl::executeLOADCTXIVAR() {
    currentStack().push(localsByCtx(getNextUInt16())[getNextUInt16()]);
  }

  void InterpreterImpl::executeSPRINT() {
    assert(!currentStack().empty());
    out << constantById(currentStack().popStringValue());
  }

  void InterpreterImpl::executeDDIV() {
    currentStack().pushDoubleValue(currentStack().popDoubleValue()  / currentStack().popDoubleValue());
  }

  void InterpreterImpl::executeSWAP() {
    currentStack().swap();
  }

  void InterpreterImpl::executeIFICMPL() {
    currentStack().popIntValue() < currentStack().popIntValue() ? jump(getNextInt16()) : (void)getNextInt16();
  }

  void InterpreterImpl::executeDUMP() {
    assert("method is not implement" == 0);
  }

  void InterpreterImpl::executeINEG() {
    assert(!currentStack().empty());
    currentStack().pushIntValue(- currentStack().popIntValue());
  }

  void InterpreterImpl::executeIMOD() {
    assert(currentStack().size() >= 2);
    currentStack().pushIntValue(currentStack().popIntValue()  % currentStack().popIntValue());
  }

  void InterpreterImpl::executeBREAK() {
    assert("method is not implement" == 0);
  }

  void InterpreterImpl::executeIFICMPE() {
    currentStack().popIntValue() == currentStack().popIntValue() ? jump(getNextInt16()) : (void)getNextInt16();
  }

  void InterpreterImpl::executeSTORESVAR3() {
    assert("method is not implement" == 0);
  }

  void InterpreterImpl::executeSTORESVAR2() {
    assert("method is not implement" == 0);
  }

  void InterpreterImpl::executeSTORESVAR1() {
    assert("method is not implement" == 0);
  }

  void InterpreterImpl::executeLOADDVAR1() {
    assert("method is not implement" == 0);
  }

  void InterpreterImpl::executeLOADDVAR0() {
    assert("method is not implement" == 0);
  }

  void InterpreterImpl::executeDLOAD0() {
    currentFrameStack().stack.pushDoubleValue(0.0);
  }

  void InterpreterImpl::executeLOADCTXSVAR() {
    assert("method is not implement" == 0);
  }

  void InterpreterImpl::executeIDIV() {
    assert(currentStack().size() >= 2);
    currentStack().pushIntValue(currentStack().popIntValue()  / currentStack().popIntValue());
  }

  void InterpreterImpl::executeDCMP() {
    assert(currentStack().size() >= 2);
    double diff = currentStack().popDoubleValue() - currentStack().popDoubleValue();
    currentStack().pushIntValue(fabs(diff) < DBL_EPSILON ? 0 : (diff > 0.0 ? 1 : -1));
  }

  void InterpreterImpl::executeSTOREDVAR() {
    assert(!currentStack().empty());
    currentLocals()[getNextUInt16()] = currentStack().pop();
  }

  void InterpreterImpl::executeIAAND() {
    assert(currentStack().size() >= 2);
    currentStack().pushIntValue(currentStack().popIntValue()  & currentStack().popIntValue());
  }

  void InterpreterImpl::executeSTOP() {
    assert("method is not implement" == 0);
  }

  void InterpreterImpl::executeLOADIVAR3() {
    assert("method is not implement" == 0);
  }

  void InterpreterImpl::executeLOADIVAR2() {
    assert("method is not implement" == 0);
  }

  void InterpreterImpl::executeLOADIVAR1() {
    assert("method is not implement" == 0);
  }

  void InterpreterImpl::executeLOADIVAR0() {
    assert("method is not implement" == 0);
  }

  void InterpreterImpl::executeLOADDVAR2() {
    assert("method is not implement" == 0);
  }

  void InterpreterImpl::executeSTORECTXDVAR() {
    assert("method is not implement" == 0);
  }

  void InterpreterImpl::executeILOAD1() {
    currentStack().pushIntValue(1);
  }

  void InterpreterImpl::executeIAXOR() {
    currentStack().pushIntValue(currentStack().popIntValue()  ^ currentStack().popIntValue());
  }

  void InterpreterImpl::executeIFICMPNE() {
    currentStack().popIntValue() != currentStack().popIntValue() ? jump(getNextInt16()) : (void)getNextInt16();
  }

  void InterpreterImpl::executeILOAD0() {
    currentStack().pushIntValue(0);
  }

  void InterpreterImpl::executeSTOREIVAR() {
    assert(!currentStack().empty());
    currentLocals()[getNextUInt16()] = currentStack().pop();
  }

  void InterpreterImpl::executeICMP() {
    assert(currentStack().size() >= 2);
    int64_t diff = currentStack().popIntValue() - currentStack().popIntValue();
    currentStack().pushIntValue((diff > 0) - (diff < 0));
  }

  void InterpreterImpl::executeISUB() {
    assert(currentStack().size() >= 2);
    currentStack().pushIntValue(currentStack().popIntValue()  - currentStack().popIntValue());
  }

  void InterpreterImpl::executeSTORECTXIVAR() {
    assert(!currentStack().empty());
    localsByCtx(getNextUInt16())[getNextUInt16()] = currentStack().pop();
  }

  void InterpreterImpl::executeCALL() {
    callFunction(getNextUInt16());
  }

  void InterpreterImpl::executeDSUB() {
    assert(currentStack().size() >= 2);
    currentStack().pushDoubleValue(currentStack().popDoubleValue()  - currentStack().popDoubleValue());
  }

  void InterpreterImpl::executeLOADDVAR3() {
    assert("method is not implement" == 0);
  }

  void InterpreterImpl::executeIFICMPG() {
    currentStack().popIntValue() > currentStack().popIntValue() ? jump(getNextInt16()) : (void)getNextInt16();
  }

  InterpreterImpl::~InterpreterImpl() {

  }

  InterpreterImpl::InterpreterImpl(): out(std::cout) {

  }

  Status *InterpreterImpl::execute(vector<Var *> &vars) {
    setInitialState();

    static void* dispatchTable[] = {
#define GEN_TABLE(c, d, l)     \
      &&label##c,

      FOR_BYTECODES(GEN_TABLE)
#undef GEN_TABLE
      &&exit
    };
    dispatchTable[BC_STOP] = &&exit;

    Status* ret = 0;



    try {
#define NEXT_INSN() goto *dispatchTable[getNextInsn()]
      NEXT_INSN();
      while (1) {
#define GEN_LABELS(c, d, l)       \
        label##c:                 \
          execute##c();           \
          NEXT_INSN();


    FOR_BYTECODES(GEN_LABELS)
#undef GEN_LABELS
        exit:
          break;
      }
#undef NEXT_INSN
    } catch (exception& e) {
      ret = new Status(e.what());
    }


    return ret == 0 ? new Status() : ret;
  }

} //mathvm