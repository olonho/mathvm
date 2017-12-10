#include "../my_include/interpreter.h"
#include <functional>

using namespace mathvm;
using namespace std;

using pVar = Var *;

Status* Interpreter::execute(vector<pVar> &vars) {

    for (pVar var : vars) {
        switch (var->type()) {
            case VT_INT:
                getVarMap<int64_t>(0)[topMostVars[var->name()]] = var->getIntValue();
                break;
            case VT_DOUBLE:
                getVarMap<double>(0)[topMostVars[var->name()]] = var->getDoubleValue();
                break;
            case VT_STRING:
                stringConstants[getVarMap<std::string>(0)[topMostVars[var->name()]]] = var->getStringValue();
                break;
            default:
                break;
        }
    }

    while (callStack.back().executionPoint < bytecode.length()) {
        Instruction instruction = bytecode.getInsn(callStack.back().executionPoint++);
        switch (instruction) {

            case BC_ILOAD:
                handleLoad<int64_t>();
                break;
            case BC_DLOAD:
                handleLoad<double>();
                break;
            case BC_SLOAD:
                handleLoad<std::string>();
                break;

            case BC_ILOAD1:
                handleLoad1<int64_t>();
                break;
            case BC_DLOAD1:
                handleLoad1<double>();
                break;
            case BC_ILOAD0:
                handleLoad0<int64_t>();
                break;
            case BC_DLOAD0:
                handleLoad0<double>();
                break;

            case BC_IADD:
                handleAdd<int64_t>();
                break;
            case BC_DADD:
                handleAdd<double>();
                break;

            case BC_ISUB:
                handleSub<int64_t>();
                break;
            case BC_DSUB:
                handleSub<double>();
                break;

            case BC_IMUL:
                handleMul<int64_t>();
                break;
            case BC_DMUL:
                handleMul<double>();
                break;

            case BC_IDIV:
                handleDiv<int64_t>();
                break;
            case BC_DDIV:
                handleDiv<double>();
                break;

            case BC_INEG:
                handleNeg<int64_t>();
                break;
            case BC_DNEG:
                handleNeg<double>();
                break;

            case BC_IMOD:
                handleMod<int64_t>();;
                break;

            case BC_IAAND:
                handleAnd<int64_t>();
                break;

            case BC_IAOR:
                handleOr<int64_t>();
                break;

            case BC_IAXOR:
                handleXor<int64_t>();
                break;

            case BC_I2D:
                handleCast<int64_t, double>();
                break;
            case BC_D2I:
                handleCast<double, int64_t>();
                break;
            case BC_S2I:
                handleCast<std::string, int64_t>();
                break;

//            case BC_STOREIVAR:
//                handleStoreVar<int64_t>();
//                break;
//            case BC_STOREDVAR:
//                handleStoreVar<double>();
//                break;
//            case BC_STORESVAR:
//                handleStoreVar<std::string>();
//                break;
            case BC_STORECTXIVAR:
                handleStoreCtxVar<int64_t>();
                break;
            case BC_STORECTXDVAR:
                handleStoreCtxVar<double>();
                break;
            case BC_STORECTXSVAR:
                handleStoreCtxVar<std::string>();
                break;

            case BC_STOREDVAR0:
                handleStoreVar0<double>();
                break;
            case BC_STOREIVAR0:
                handleStoreVar0<int64_t>();
                break;
            case BC_STORESVAR0:
                handleStoreVar0<std::string>();
                break;

            case BC_STOREDVAR1:
                handleStoreVar1<double>();
                break;
            case BC_STOREIVAR1:
                handleStoreVar1<int64_t>();
                break;
            case BC_STORESVAR1:
                handleStoreVar1<std::string>();
                break;

            case BC_LOADDVAR0:
                handleLoadVar0<double>();
                break;
            case BC_LOADIVAR0:
                handleLoadVar0<int64_t>();
                break;
            case BC_LOADSVAR0:
                handleLoadVar0<std::string>();
                break;

            case BC_LOADDVAR1:
                handleLoadVar1<double>();
                break;
            case BC_LOADIVAR1:
                handleLoadVar1<int64_t>();
                break;
            case BC_LOADSVAR1:
                handleLoadVar1<std::string>();
                break;

//            case BC_LOADDVAR:
//                handleLoadVar<double>();
//                break;
//            case BC_LOADIVAR:
//                handleLoadVar<int64_t>();
//                break;
//            case BC_LOADSVAR:
//                handleLoadVar<std::string>();
//                break;
            case BC_LOADCTXDVAR:
                handleLoadCtxVar<double>();
                break;
            case BC_LOADCTXIVAR:
                handleLoadCtxVar<int64_t>();
                break;
            case BC_LOADCTXSVAR:
                handleLoadCtxVar<std::string>();
                break;

            case BC_IPRINT:
                handlePrint<int64_t>();
                break;
            case BC_DPRINT:
                handlePrint<double>();
                break;
            case BC_SPRINT:
                handlePrint<std::string>();
                break;

            case BC_IFICMPGE:
                handleCmpge();
                break;
            case BC_IFICMPLE:
                handleCmple();
                break;
            case BC_IFICMPG:
                handleCmpg();
                break;
            case BC_IFICMPL:
                handleCmpl();
                break;
            case BC_IFICMPE:
                handleCmpe();
                break;
            case BC_IFICMPNE:
                handleCmpne();
                break;

            case BC_JA:
                handleJa();
                break;
            case BC_SWAP:
                handleSwap();
                break;
            case BC_POP:
                handlePop();
                break;
            case BC_CALL:
                handleCall();
                break;
            case BC_CALLNATIVE:
                handleCallNative();
                break;
            case BC_RETURN:
                handleReturn();
                break;

            case BC_ICMP:
                handleIcmp();
                break;

            default:
                size_t length;
                const char* name = bytecodeName(instruction, &length);
                std::cout << "UNKNOWN COMMAND: " << name << std::endl;
                exit(1);
        }
    }

    for (pVar var : vars) {
        switch (var->type()) {
            case VT_INT:
                var->setIntValue(getVarMap<int64_t>(0)[topMostVars[var->name()]]);
                break;
            case VT_DOUBLE:
                var->setDoubleValue(getVarMap<double>(0)[topMostVars[var->name()]]);
                break;
            case VT_STRING: {
                const char * val = getString(getVarMap<std::string>(0)[topMostVars[var->name()]]);
                var->setStringValue(val);
                break;
            }
            default:
                break;
        }

    }

    return Status::Ok();
}

void Interpreter::disassemble(std::ostream &out, mathvm::FunctionFilter *filter) {
    bytecode.dump(out);
}