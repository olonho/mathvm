//
// Created by Vadim Lomshakov on 10/26/13.
// Copyright (c) 2013 spbau. All rights reserved.
//


#include <math.h>
#include "InterpreterImpl.h"
#include "MSMemoryManager.h"

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

  void InterpreterImpl::executeAALOAD() {
    int64_t idx = currentStack().popIntValue();
    assert(idx >= 0);
    sysint_t * arrayref = (sysint_t *) currentStack().popRefValue();
    assert(arrayref != 0);

    currentStack().pushRefValue((void*)arrayref[idx]);
  }

  void InterpreterImpl::executeDALOAD() {
    int64_t idx = currentStack().popIntValue();
    assert(idx >= 0);
    double * arrayref = (double *) currentStack().popRefValue();
    assert(arrayref != 0);

    currentStack().pushDoubleValue(arrayref[idx]);
  }

  void InterpreterImpl::executeIALOAD() {
    int64_t idx = currentStack().popIntValue();
    assert(idx >= 0);
    int64_t * arrayref = (int64_t *) currentStack().popRefValue();
    assert(arrayref != 0);

    currentStack().pushIntValue(arrayref[idx]);
  }

  void InterpreterImpl::executeAASTORE() {
    void* value = currentStack().popRefValue();
    assert(value != 0);
    int64_t idx = currentStack().popIntValue();
    assert(idx >= 0);
    sysint_t * arrayref = (sysint_t *) currentStack().popRefValue();
    assert(arrayref != 0);

    arrayref[idx] = (sysint_t)value;
  }

  void InterpreterImpl::executeIASTORE() {
    int64_t value = currentStack().popIntValue();
    int64_t idx = currentStack().popIntValue();
    assert(idx >= 0);
    int64_t * arrayref = (int64_t *) currentStack().popRefValue();
    assert(arrayref != 0);

    arrayref[idx] = value;
  }

  void InterpreterImpl::executeDASTORE() {
    double value = currentStack().popDoubleValue();
    int64_t idx = currentStack().popIntValue();
    assert(idx >= 0);
    double * arrayref = (double *) currentStack().popRefValue();
    assert(arrayref != 0);

    arrayref[idx] = value;
  }

  void InterpreterImpl::executeLOADAVAR() {
    currentStack().push(currentLocals()[getNextUInt16()]);
  }

  void InterpreterImpl::executeSTOREAVAR() {
    assert(!currentStack().empty());
    currentLocals()[getNextUInt16()] = currentStack().pop();
  }

  void InterpreterImpl::executeLOADCTXAVAR() {
    currentStack().push(localsByCtx(getNextUInt16())[getNextUInt16()]);
  }

  void InterpreterImpl::executeSTORECTXAVAR() {
    assert(!currentStack().empty());
    localsByCtx(getNextUInt16())[getNextUInt16()] = currentStack().pop();
  }

  void InterpreterImpl::executeINEWARRAY() {
    currentStack().pushRefValue(MSMemoryManager::mm().alloc(RefMetaData(currentStack().popIntValue(), VT_INT)));
  }

  void InterpreterImpl::executeDNEWARRAY() {
    currentStack().pushRefValue(MSMemoryManager::mm().alloc(RefMetaData(currentStack().popIntValue(), VT_DOUBLE)));
  }

  static
  void* allocateMultiArray(vector<int64_t> const& dims, TypeTag primitive, int deep) {
    if (deep == 0)
      return MSMemoryManager::mm().alloc(RefMetaData(dims[0], primitive));


    sysint_t* ptr;
    ptr = (sysint_t*) MSMemoryManager::mm().alloc(RefMetaData(dims[deep], VT_REF));
    for (int i = 0; i != dims[deep]; ++i) {
      ptr[i] = (sysint_t) allocateMultiArray(dims, primitive, deep - 1);
    }
    return ptr;
  }

  void InterpreterImpl::executeIMULTIANEWARRYA() {
    uint16_t countDims = getNextUInt16();
    vector<int64_t> dims;
    size_t size = sizeof(int64_t);
    size_t hsize = 0;
    dims.push_back(currentStack().popIntValue()), size *= dims.back();
    for (uint16_t i = 1; i != countDims; ++i)
      dims.push_back(currentStack().popIntValue()), size *= dims.back(), hsize += dims.back() * sizeof(sysint_t);
    size += hsize;

    if (MSMemoryManager::mm().getFreeMem() < size) {
      MSMemoryManager::mm().collect();
      if (MSMemoryManager::mm().getFreeMem() < size)
        throw runtime_error("Out of memory");
    }
    void * ptr = allocateMultiArray(dims, VT_INT, dims.size() - 1);

    currentStack().pushRefValue(ptr);
  }

  void InterpreterImpl::executeDMULTIANEWARRYA() {
    uint16_t countDims = getNextUInt16();
    vector<int64_t> dims;
    size_t size = sizeof(double);
    size_t hsize = 0;
    dims.push_back(currentStack().popIntValue()), size *= dims.back();
    for (uint16_t i = 1; i != countDims; ++i)
      dims.push_back(currentStack().popIntValue()), size *= dims.back(), hsize += dims.back() * sizeof(sysint_t);
    size += hsize;

    if (MSMemoryManager::mm().getFreeMem() < size) {
      MSMemoryManager::mm().collect();
      if (MSMemoryManager::mm().getFreeMem() < size)
        throw runtime_error("Out of memory");
    }
    void * ptr = allocateMultiArray(dims, VT_DOUBLE, dims.size() - 1);

    currentStack().pushRefValue(ptr);
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
    Signature const ** signature = (Signature const **) new Signature*;
    string const ** name = (string const **) new string*;
    void const * code = nativeById(getNextUInt16(), signature, name);
    delete name;

    VarType returnType = (**signature)[0].first;

    Compiler c;

    switch (returnType.tag()) {
      case VT_DOUBLE:
        c.newFunction(CALL_CONV_DEFAULT, FunctionBuilder0<double>());
        break;
      case VT_INT:
        c.newFunction(CALL_CONV_DEFAULT, FunctionBuilder0<int64_t>());
        break;
      case VT_STRING:
        c.newFunction(CALL_CONV_DEFAULT, FunctionBuilder0<char*>());
        break;
      case VT_VOID:
        c.newFunction(CALL_CONV_DEFAULT, FunctionBuilder0<void>());
        break;
      case VT_INVALID:
      default:
        throw std::runtime_error("wrong return type");
    }
    c.getFunction()->setHint(FUNCTION_HINT_NAKED, true);

    size_t countArgs = (**signature).size() - 1;
    std::vector<BaseVar> args;
    assert(currentLocals().size() == countArgs);
    for (size_t i = 0; i != countArgs; ++i) {
      switch ((**signature)[i + 1].first.tag()) {
        case VT_INT: {
          GPVar arg(c.newGP());
          c.mov(arg, Imm(currentLocals()[i].getIntValue()));
          args.push_back(arg);
          break;
        }
        case VT_DOUBLE: {
          XMMVar arg(c.newXMM(VARIABLE_TYPE_XMM_1D));
          GPVar tmp(c.newGP(VARIABLE_TYPE_INT64));
          c.mov(tmp, Imm(currentLocals()[i].getIntValue()));
          c.movq(arg, tmp);
          c.unuse(tmp);
          args.push_back(arg);
          break;
        }
        case VT_STRING: {
          GPVar arg(c.newGP());
          int64_t addr = (int64_t) getStringConstant(currentLocals()[i].getStringValue());
          // if you wanna to modify string, it's your problem)
          c.mov(arg, Imm(addr));
          args.push_back(arg);
          break;
        }
        case VT_VOID: break;
        case VT_INVALID:
        default:
          throw std::runtime_error("wrong arg type");
      }
    }

    XMMVar retVal(c.newXMM(VARIABLE_TYPE_XMM_1D));

    // Call a function.
    GPVar address(c.newGP());
    c.mov(address, imm((sysint_t)code));
    ECall* ctx = c.call(address);


    FunctionBuilderX definition = FunctionBuilderX();
    switch (returnType.tag()) {
      case VT_DOUBLE:
        definition.setReturnValue<double>();
        break;
      case VT_INT:
        definition.setReturnValue<int64_t>();
        break;
      case VT_STRING:
        definition.setReturnValue<char*>();
        break;
      case VT_VOID:
        definition.setReturnValue<void>();
        break;
      case VT_INVALID:
      default:
        throw std::runtime_error("wrong return type");
    }

    for (size_t i = 0; i != countArgs; ++i) {
      switch ((**signature)[i + 1].first.tag()) {
        case VT_DOUBLE:
          definition.addArgument<double>();
          break;
        case VT_INT:
          definition.addArgument<int64_t>();
          break;
        case VT_STRING:
          definition.addArgument<char*>();
          break;
        case VT_VOID:
          definition.addArgument<void>();
          break;
        case VT_INVALID:
        default:
          throw std::runtime_error("wrong return type");
      }
    }
    ctx->setPrototype(CALL_CONV_DEFAULT, definition);

    for(size_t i = 0; i != countArgs; ++i)
      ctx->setArgument(i, args[i]);

    ctx->setReturn(retVal);

    c.ret(retVal);
    c.endFunction();

    // Make the function.
    void* fn = c.make();

    switch (returnType.tag()) {
      case VT_DOUBLE:
        currentStack().pushDoubleValue(function_cast<double(*)()>(fn)());
        break;
      case VT_INT:
        currentStack().pushIntValue(function_cast<int64_t(*)()>(fn)());
        break;
      case VT_STRING: {
        char* ptr = function_cast<char*(*)()>(fn)();
        currentStack().pushStringValue(makeExternalString(ptr));
      break; }
      case VT_VOID:
        function_cast<void(*)()>(fn)();
        break;
      case VT_INVALID:
      default:
        throw std::runtime_error("wrong return type");
    }


    // Free the generated function if it's not needed anymore.
    MemoryManager::getGlobal()->free(fn);
    delete signature;
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
    out << getStringConstant(currentStack().popStringValue());//constantById(currentStack().popStringValue());
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
    MSMemoryManager::mm().setClient(this);
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