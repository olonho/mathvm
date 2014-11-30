#include "interpretercode.h"
#include <cstdio>


namespace mathvm {

int64_t InterpreterCodeImpl::compare(double upper, double lower) {
    if (upper == lower) {
        return 0;
    }

    return upper > lower ? 1 : -1;
}

int64_t InterpreterCodeImpl::compare(int64_t upper, int64_t lower) {
    if (upper == lower) {
        return 0;
    }

    return upper > lower ? 1 : -1;
}

InterpreterCodeImpl::~InterpreterCodeImpl() {

}

Status* InterpreterCodeImpl::execute(vector<Var*> &vars)
{
    BytecodeFunction* bcFunction = dynamic_cast<BytecodeFunction*>(functionByName("<top>"));
    try {
        executeBytecodeFunction(bcFunction);
    }
    catch (InterpretationException& exception) {
        return Status::Error(exception.what(), exception.bytecodePosition());
    }

    return Status::Ok();
}

void InterpreterCodeImpl::disassemble(ostream &out, FunctionFilter *filter)
{
    Code::disassemble(out, filter);
}

void InterpreterCodeImpl::executeBytecode() {
    while(bytecodeIndex() != bytecode()->length()) {
        Instruction insn = static_cast<Instruction>(bytecode()->get(bytecodeIndex()));
        executeBytecodeInsn(insn);
    }
}

void InterpreterCodeImpl::executeBytecodeInsn(Instruction insn) {
    switch(insn) {
#define CASE_INSN(b, d, l)         \
    case BC_##b:                   \
    execute##b();              \
    break;

    FOR_BYTECODES(CASE_INSN)
#undef CASE_INSN

    default:
        throw InterpretationException("Incorrect instruction");
    break;
    }
}

Bytecode* InterpreterCodeImpl::bytecode() {
    return _bytecodes.back();
}

void InterpreterCodeImpl::addNewBytecode(Bytecode *bytecode) {
    _bytecodes.push_back(bytecode);

    if (_bytecodes.size() > 10) {
        assert(_bytecodes.back() == _bytecodes[_bytecodes.size() - 1]);
    }

    _indexes.push_back(0);
}

void InterpreterCodeImpl::removeLastBytecode() {
    _bytecodes.pop_back();
    _indexes.pop_back();
}

uint32_t InterpreterCodeImpl::bytecodeIndex() {
    return _indexes.back();
}

void InterpreterCodeImpl::shiftBytecodeIndex(int16_t shift) {
    _indexes.back() += shift;
}

void InterpreterCodeImpl::setBytecodeIndex(uint32_t index) {
    _indexes.back() = index;
}

void InterpreterCodeImpl::executeINVALID() {
    throw InterpretationException("INVALID instruction");
}

void InterpreterCodeImpl::executeDLOAD()
{
    shiftBytecodeIndex(1);
    double d = bytecode()->getDouble(bytecodeIndex());
    shiftBytecodeIndex(sizeof(double));
    _stack.pushDouble(d);
}

void InterpreterCodeImpl::executeILOAD()
{
    shiftBytecodeIndex(1);
    int64_t i = bytecode()->getTyped<int64_t>(bytecodeIndex());
    shiftBytecodeIndex(sizeof(int64_t));
    _stack.pushInt(i);
}

void InterpreterCodeImpl::executeSLOAD()
{
    shiftBytecodeIndex(1);
    uint16_t id = bytecode()->getUInt16(bytecodeIndex());
    shiftBytecodeIndex(sizeof(uint16_t));
    _stack.pushStringId(id);
}

void InterpreterCodeImpl::executeDLOAD0()
{
    shiftBytecodeIndex(1);
    _stack.pushDouble(0);
}

void InterpreterCodeImpl::executeILOAD0()
{
    shiftBytecodeIndex(1);
    _stack.pushInt(0);
}

void InterpreterCodeImpl::executeSLOAD0()
{
    shiftBytecodeIndex(1);
    _stack.pushStringId(0);
}

void InterpreterCodeImpl::executeDLOAD1()
{
    shiftBytecodeIndex(1);
    _stack.pushDouble(1.0);
}

void InterpreterCodeImpl::executeILOAD1()
{
    shiftBytecodeIndex(1);
    _stack.pushInt(1);
}

void InterpreterCodeImpl::executeDLOADM1()
{
    shiftBytecodeIndex(1);
    _stack.pushDouble(-1.0);
}

void InterpreterCodeImpl::executeILOADM1()
{
    shiftBytecodeIndex(1);
    _stack.pushInt(-1);
}

void InterpreterCodeImpl::executeDADD()
{
    shiftBytecodeIndex(1);
    double d1 = _stack.popDouble();
    double d2 = _stack.popDouble();
    double dres = d1 + d2;
    _stack.pushDouble(dres);
}

void InterpreterCodeImpl::executeIADD()
{
    shiftBytecodeIndex(1);
    int64_t i1 = _stack.popInt();
    int64_t i2 = _stack.popInt();
    int64_t ires = i1 + i2;
    _stack.pushInt(ires);
}

void InterpreterCodeImpl::executeDSUB()
{
    shiftBytecodeIndex(1);
    double d1 = _stack.popDouble();
    double d2 = _stack.popDouble();
    double dres = d1 - d2;
    _stack.pushDouble(dres);
}

void InterpreterCodeImpl::executeISUB()
{
    shiftBytecodeIndex(1);
    int64_t i1 = _stack.popInt();
    int64_t i2 = _stack.popInt();
    int64_t ires = i1 - i2;
    _stack.pushInt(ires);
}

void InterpreterCodeImpl::executeDMUL()
{
    shiftBytecodeIndex(1);
    double d1 = _stack.popDouble();
    double d2 = _stack.popDouble();
    double dres = d1 * d2;
    _stack.pushDouble(dres);
}

void InterpreterCodeImpl::executeIMUL()
{
    shiftBytecodeIndex(1);
    int64_t i1 = _stack.popInt();
    int64_t i2 = _stack.popInt();
    int64_t ires = i1 * i2;
    _stack.pushInt(ires);
}

void InterpreterCodeImpl::executeDDIV()
{
    shiftBytecodeIndex(1);
    double d1 = _stack.popDouble();
    double d2 = _stack.popDouble();
    if (!d2) {
        throw InterpretationException("Division by zero");
    }
    double dres = d1 / d2;
    _stack.pushDouble(dres);
}

void InterpreterCodeImpl::executeIDIV()
{
    shiftBytecodeIndex(1);
    int64_t i1 = _stack.popInt();
    int64_t i2 = _stack.popInt();
    if (!i2) {
        throw InterpretationException("Division by zero");
    }
    int64_t ires = i1 / i2;
    _stack.pushInt(ires);
}

void InterpreterCodeImpl::executeIMOD()
{
    shiftBytecodeIndex(1);
    int64_t i1 = _stack.popInt();
    int64_t i2 = _stack.popInt();
    int64_t ires = i1 % i2;
    _stack.pushInt(ires);
}

void InterpreterCodeImpl::executeDNEG()
{
    shiftBytecodeIndex(1);
    double d = _stack.popDouble();
    double dneg = -d;
    _stack.pushDouble(dneg);
}

void InterpreterCodeImpl::executeINEG()
{
    shiftBytecodeIndex(1);
    int64_t i = _stack.popInt();
    int64_t ineg = -i;
    _stack.pushInt(ineg);
}

void InterpreterCodeImpl::executeIAOR()
{
    shiftBytecodeIndex(1);
    int64_t i1 = _stack.popInt();
    int64_t i2 = _stack.popInt();
    int64_t iaor = i1 | i2;
    _stack.pushInt(iaor);
}

void InterpreterCodeImpl::executeIAAND()
{
    shiftBytecodeIndex(1);
    int64_t i1 = _stack.popInt();
    int64_t i2 = _stack.popInt();
    int64_t iaand = i1 & i2;
    _stack.pushInt(iaand);
}

void InterpreterCodeImpl::executeIAXOR()
{
    shiftBytecodeIndex(1);
    int64_t i1 = _stack.popInt();
    int64_t i2 = _stack.popInt();
    int64_t iaxor = i1 ^ i2;
    _stack.pushInt(iaxor);
}

void InterpreterCodeImpl::executeIPRINT()
{
    shiftBytecodeIndex(1);
    int64_t i = _stack.popInt();
    std::cout << i;
}

void InterpreterCodeImpl::executeDPRINT()
{
    shiftBytecodeIndex(1);
    double d = _stack.popDouble();
    std::cout << d;
}

void InterpreterCodeImpl::executeSPRINT()
{
    shiftBytecodeIndex(1);
    uint16_t id = _stack.popStringId();
    std::cout << constantById(id);
}

void InterpreterCodeImpl::executeI2D()
{
    shiftBytecodeIndex(1);
    int64_t i = _stack.popInt();
    double d = static_cast<double>(i);
    _stack.pushDouble(d);
}

void InterpreterCodeImpl::executeD2I()
{
    shiftBytecodeIndex(1);
    double d = _stack.popDouble();
    int64_t i = static_cast<int64_t>(d);
    _stack.pushInt(i);
}

void InterpreterCodeImpl::executeS2I()
{
    shiftBytecodeIndex(1);
    throw InterpretationException("Couldn't cast \"string\" to \"int\"");
}

void InterpreterCodeImpl::executeSWAP()
{
    shiftBytecodeIndex(1);
    _stack.swapTwoUpperElements();
}

void InterpreterCodeImpl::executePOP()
{
    shiftBytecodeIndex(1);
    _stack.popElement();
}

void InterpreterCodeImpl::executeLOADDVAR0()
{
    shiftBytecodeIndex(1);
    double d = _contextManager.loadDoubleFromVar(0);
    _stack.pushDouble(d);
}

void InterpreterCodeImpl::executeLOADDVAR1()
{
    shiftBytecodeIndex(1);
    double d = _contextManager.loadDoubleFromVar(1);
    _stack.pushDouble(d);
}

void InterpreterCodeImpl::executeLOADDVAR2()
{
    shiftBytecodeIndex(1);
    double d = _contextManager.loadDoubleFromVar(2);
    _stack.pushDouble(d);
}

void InterpreterCodeImpl::executeLOADDVAR3()
{
    shiftBytecodeIndex(1);
    double d = _contextManager.loadDoubleFromVar(3);
    _stack.pushDouble(d);
}

void InterpreterCodeImpl::executeLOADIVAR0()
{
    shiftBytecodeIndex(1);
    int64_t i = _contextManager.loadIntFromVar(0);
    _stack.pushInt(i);
}

void InterpreterCodeImpl::executeLOADIVAR1()
{
    shiftBytecodeIndex(1);
    int64_t i = _contextManager.loadIntFromVar(1);
    _stack.pushInt(i);
}

void InterpreterCodeImpl::executeLOADIVAR2()
{
    shiftBytecodeIndex(1);
    int64_t i = _contextManager.loadIntFromVar(2);
    _stack.pushInt(i);
}

void InterpreterCodeImpl::executeLOADIVAR3()
{
    shiftBytecodeIndex(1);
    int64_t i = _contextManager.loadIntFromVar(3);
    _stack.pushInt(i);
}

void InterpreterCodeImpl::executeLOADSVAR0()
{
    shiftBytecodeIndex(1);
    uint16_t stringId = _contextManager.loadStringIdFromVar(0);
    _stack.pushStringId(stringId);
}

void InterpreterCodeImpl::executeLOADSVAR1()
{
    shiftBytecodeIndex(1);
    uint16_t stringId = _contextManager.loadStringIdFromVar(1);
    _stack.pushStringId(stringId);
}

void InterpreterCodeImpl::executeLOADSVAR2()
{
    shiftBytecodeIndex(1);
    uint16_t stringId = _contextManager.loadStringIdFromVar(2);
    _stack.pushStringId(stringId);
}

void InterpreterCodeImpl::executeLOADSVAR3()
{
    shiftBytecodeIndex(1);
    uint16_t stringId = _contextManager.loadStringIdFromVar(3);
    _stack.pushStringId(stringId);
}

void InterpreterCodeImpl::executeSTOREDVAR0()
{
    shiftBytecodeIndex(1);
    double d = _stack.popDouble();
    _contextManager.storeDoubleToVar(0, d);
}

void InterpreterCodeImpl::executeSTOREDVAR1()
{
    shiftBytecodeIndex(1);
    double d = _stack.popDouble();
    _contextManager.storeDoubleToVar(1, d);
}

void InterpreterCodeImpl::executeSTOREDVAR2()
{
    shiftBytecodeIndex(1);
    double d = _stack.popDouble();
    _contextManager.storeDoubleToVar(2, d);
}

void InterpreterCodeImpl::executeSTOREDVAR3()
{
    shiftBytecodeIndex(1);
    double d = _stack.popDouble();
    _contextManager.storeDoubleToVar(3, d);
}

void InterpreterCodeImpl::executeSTOREIVAR0()
{
    shiftBytecodeIndex(1);
    int64_t i = _stack.popInt();
    _contextManager.storeIntToVar(0, i);
}

void InterpreterCodeImpl::executeSTOREIVAR1()
{
    shiftBytecodeIndex(1);
    int64_t i = _stack.popInt();
    _contextManager.storeIntToVar(1, i);
}

void InterpreterCodeImpl::executeSTOREIVAR2()
{
    shiftBytecodeIndex(1);
    int64_t i = _stack.popInt();
    _contextManager.storeIntToVar(2, i);
}

void InterpreterCodeImpl::executeSTOREIVAR3()
{
    shiftBytecodeIndex(1);
    int64_t i = _stack.popInt();
    _contextManager.storeIntToVar(3, i);
}

void InterpreterCodeImpl::executeSTORESVAR0()
{
    shiftBytecodeIndex(1);
    uint16_t stringId = _stack.popStringId();
    _contextManager.storeStringIdToVar(0, stringId);
}

void InterpreterCodeImpl::executeSTORESVAR1()
{
    shiftBytecodeIndex(1);
    uint16_t stringId = _stack.popStringId();
    _contextManager.storeStringIdToVar(1, stringId);
}

void InterpreterCodeImpl::executeSTORESVAR2()
{
    shiftBytecodeIndex(1);
    uint16_t stringId = _stack.popStringId();
    _contextManager.storeStringIdToVar(2, stringId);
}

void InterpreterCodeImpl::executeSTORESVAR3()
{
    shiftBytecodeIndex(1);
    uint16_t stringId = _stack.popStringId();
    _contextManager.storeStringIdToVar(3, stringId);
}

void InterpreterCodeImpl::readVarId(uint16_t& varId) {
    varId = bytecode()->getUInt16(bytecodeIndex());
    shiftBytecodeIndex(2);
}

void InterpreterCodeImpl::executeLOADDVAR()
{
    shiftBytecodeIndex(1);
    uint16_t varId;
    readVarId(varId);
    double d = _contextManager.loadDoubleFromVar(varId);
    _stack.pushDouble(d);
}

void InterpreterCodeImpl::executeLOADIVAR()
{
    shiftBytecodeIndex(1);
    uint16_t varId;
    readVarId(varId);
    int64_t i = _contextManager.loadIntFromVar(varId);
    _stack.pushInt(i);
}

void InterpreterCodeImpl::executeLOADSVAR()
{
    shiftBytecodeIndex(1);
    uint16_t varId;
    readVarId(varId);
    uint16_t stringId = _contextManager.loadStringIdFromVar(varId);
    _stack.pushInt(stringId);
}

void InterpreterCodeImpl::executeSTOREDVAR()
{
    shiftBytecodeIndex(1);
    uint16_t varId;
    readVarId(varId);

    double d = _stack.popDouble();
    _contextManager.storeDoubleToVar(varId, d);
}

void InterpreterCodeImpl::executeSTOREIVAR()
{
    shiftBytecodeIndex(1);
    uint16_t varId;
    readVarId(varId);

    int64_t i = _stack.popInt();
    _contextManager.storeIntToVar(varId, i);
}

void InterpreterCodeImpl::executeSTORESVAR()
{
    shiftBytecodeIndex(1);
    uint16_t varId;
    readVarId(varId);

    uint16_t stringId = _stack.popStringId();
    _contextManager.storeStringIdToVar(varId, stringId);
}

void InterpreterCodeImpl::readCtxIdVarId(uint16_t& outContextId, uint16_t& outVarId) {
    uint16_t contextId = bytecode()->getUInt16(bytecodeIndex());
    shiftBytecodeIndex(2);
    uint16_t varId = bytecode()->getUInt16(bytecodeIndex());
    shiftBytecodeIndex(2);

    outContextId = contextId;
    outVarId = varId;
}

void InterpreterCodeImpl::executeLOADCTXDVAR()
{
    shiftBytecodeIndex(1);
    uint16_t contextId, varId;
    readCtxIdVarId(contextId, varId);

    double d = _contextManager.loadDoubleFromCtxVar(contextId, varId);
    _stack.pushDouble(d);
}

void InterpreterCodeImpl::executeLOADCTXIVAR()
{
    shiftBytecodeIndex(1);
    uint16_t contextId, varId;
    readCtxIdVarId(contextId, varId);

    int64_t i = _contextManager.loadIntFromCtxVar(contextId, varId);
    _stack.pushInt(i);
}

void InterpreterCodeImpl::executeLOADCTXSVAR()
{
    shiftBytecodeIndex(1);
    uint16_t contextId, varId;
    readCtxIdVarId(contextId, varId);

    uint16_t constantId = _contextManager.loadStringIdFromCtxVar(contextId, varId);
    _stack.pushStringId(constantId);
}

void InterpreterCodeImpl::executeSTORECTXDVAR()
{
    shiftBytecodeIndex(1);
    uint16_t contextId, varId;
    readCtxIdVarId(contextId, varId);

    double d = _stack.popDouble();
    _contextManager.storeDoubleToCtxVar(contextId, varId, d);
}

void InterpreterCodeImpl::executeSTORECTXIVAR()
{
    shiftBytecodeIndex(1);
    uint16_t contextId, varId;
    readCtxIdVarId(contextId, varId);
    int64_t i = _stack.popInt();
    _contextManager.storeIntToCtxVar(contextId, varId, i);
}

void InterpreterCodeImpl::executeSTORECTXSVAR() {
    shiftBytecodeIndex(1);
    uint16_t contextId, varId;
    readCtxIdVarId(contextId, varId);
    uint16_t stringId = _stack.popStringId();
    _contextManager.storeStringIdToCtxVar(contextId, varId, stringId);
}

void InterpreterCodeImpl::executeDCMP()
{
    shiftBytecodeIndex(1);
    double d1 = _stack.popDouble();
    double d2 = _stack.popDouble();
    int64_t cmp = InterpreterCodeImpl::compare(d1, d2);
    _stack.pushInt(cmp);
}

void InterpreterCodeImpl::executeICMP()
{
    shiftBytecodeIndex(1);
    int64_t i1 = _stack.popInt();
    int64_t i2 = _stack.popInt();
    int64_t cmp = InterpreterCodeImpl::compare(i1, i2);
    _stack.pushInt(cmp);
}

void InterpreterCodeImpl::executeJA()
{
    shiftBytecodeIndex(1);
    int16_t offset = bytecode()->getInt16(bytecodeIndex());
    shiftBytecodeIndex(offset);
}

void InterpreterCodeImpl::executeIFICMPNE() {
    shiftBytecodeIndex(1);
    int64_t upper = _stack.popInt();
    int64_t lower = _stack.popInt();
    if (upper != lower) {
        int16_t jump = bytecode()->getInt16(bytecodeIndex());
        shiftBytecodeIndex(jump);
    }
    else {
        shiftBytecodeIndex(2);
    }
}

void InterpreterCodeImpl::executeIFICMPE() {
    shiftBytecodeIndex(1);
    int64_t upper = _stack.popInt();
    int64_t lower = _stack.popInt();
    if (upper == lower) {
        int16_t jump = bytecode()->getInt16(bytecodeIndex());
        shiftBytecodeIndex(jump);
    }
    else {
        shiftBytecodeIndex(2);
    }
}

void InterpreterCodeImpl::executeIFICMPG() {
    shiftBytecodeIndex(1);
    int64_t upper = _stack.popInt();
    int64_t lower = _stack.popInt();
    if (upper > lower) {
        int16_t jump = bytecode()->getInt16(bytecodeIndex());
        shiftBytecodeIndex(jump);
    }
    else {
        shiftBytecodeIndex(2);
    }
}

void InterpreterCodeImpl::executeIFICMPGE() {
    shiftBytecodeIndex(1);
    int64_t upper = _stack.popInt();
    int64_t lower = _stack.popInt();
    if (upper >= lower) {
        int16_t jump = bytecode()->getInt16(bytecodeIndex());
        shiftBytecodeIndex(jump);
    }
    else {
        shiftBytecodeIndex(2);
    }
}

void InterpreterCodeImpl::executeIFICMPL( ) {
    shiftBytecodeIndex(1);
    int64_t upper = _stack.popInt();
    int64_t lower = _stack.popInt();
    if (upper < lower) {
        int16_t jump = bytecode()->getInt16(bytecodeIndex());
        shiftBytecodeIndex(jump);
    }
    else {
        shiftBytecodeIndex(2);
    }
}

void InterpreterCodeImpl::executeIFICMPLE() {
    shiftBytecodeIndex(1);
    int64_t upper = _stack.popInt();
    int64_t lower = _stack.popInt();
    if (upper <= lower) {
        int16_t jump = bytecode()->getInt16(bytecodeIndex());
        shiftBytecodeIndex(jump);
    }
    else {
        shiftBytecodeIndex(2);
    }
}

void InterpreterCodeImpl::executeDUMP()
{
    shiftBytecodeIndex(1);
    ContextVar topElement = _stack.getElement();
    switch(topElement.type()) {
    case VT_DOUBLE:
        std::cout << topElement.doubleValue();
        break;
    case VT_INT:
        std::cout << topElement.intValue();
        break;
    case VT_STRING:
        std::cout << constantById(topElement.stringIdValue()).c_str();
        break;
    default:
        throw InterpretationException("Incorrect type for DUMP");
        break;
    }
}

void InterpreterCodeImpl::executeSTOP()
{
    shiftBytecodeIndex(1);
    throw InterpretationException("Stopped");
}

void InterpreterCodeImpl::executeCALL()
{
    shiftBytecodeIndex(1);
    uint16_t funcId = bytecode()->getUInt16(bytecodeIndex());
    shiftBytecodeIndex(2);
    BytecodeFunction* bcFunction = dynamic_cast<BytecodeFunction*>(functionById(funcId));

    executeBytecodeFunction(bcFunction);

}

void InterpreterCodeImpl::executeBytecodeFunction(BytecodeFunction* bcFunction) {
    _contextManager.addContext(bcFunction->id());

    Bytecode* bytecode = bcFunction->bytecode();
    addNewBytecode(bytecode);
    executeBytecode();
    removeLastBytecode();

    _contextManager.removeContext(bcFunction->id());
}

void InterpreterCodeImpl::executeCALLNATIVE()
{
    shiftBytecodeIndex(1);
    uint16_t nativeId = bytecode()->getUInt16(bytecodeIndex());
    shiftBytecodeIndex(2);

    const Signature* signature;
    const string* name;
    const void* code = nativeById(nativeId, &signature, &name);
    executeNativeFunction(name, signature, code);
}

void InterpreterCodeImpl::executeNativeFunction(const string* name, const Signature* signature, const void* code) {

}

void InterpreterCodeImpl::executeRETURN()
{
    shiftBytecodeIndex(1);
    setBytecodeIndex(bytecode()->length());
}

void InterpreterCodeImpl::executeBREAK()
{
    shiftBytecodeIndex(1);
}



}
