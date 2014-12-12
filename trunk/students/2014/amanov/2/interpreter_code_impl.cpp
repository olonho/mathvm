#include "interpreter_code_impl.h"
#include <stack>

namespace mathvm {

const size_t InterpreterCodeImpl::MAX_STACK_SIZE = 256 * 1024 * 1024;

InterpreterCodeImpl::InterpreterCodeImpl()
    : m_pc(0)
    , m_funId(0)
    , m_currentLocalNum(0)
    , m_fun_map_start(0)
    , m_bytecode(0)
{
    m_callStack.reserve(MAX_STACK_SIZE / sizeof(CallFrame));
    m_stack.reserve(MAX_STACK_SIZE / sizeof(StackValue));
    m_context.reserve(MAX_STACK_SIZE / sizeof(StackValue));
    m_tos = &m_stack[0];
    m_currentCtx = &m_context[0];
}

const char *InterpreterCodeImpl::nextString()
{
    uint16_t id = getNext<uint16_t>();
    return constantById(id).c_str();
}

void InterpreterCodeImpl::doIntBinaryOp(TokenKind op)
{
    int64_t u = popFromStack();
    int64_t l = popFromStack();
    switch (op) {
        case tMOD: pushToStack(u % l); return;
        case tAAND: pushToStack(u & l); return;
        case tAOR : pushToStack(u | l); return;
        case tAXOR : pushToStack(u ^ l); return;
        default : break;
    }
    assert(false);
}

void InterpreterCodeImpl::loadContextVar()
{
    uint16_t ctxId = getNext<uint16_t>();
    uint16_t varId = getNext<uint16_t>();
    pushToStack(*(*(m_fun_map_start +ctxId) + varId));
}

void InterpreterCodeImpl::storeContextVar()
{
    uint16_t ctxId = getNext<uint16_t>();
    uint16_t varId = getNext<uint16_t>();
    *(*(m_fun_map_start + ctxId) + varId) = popFromStack();
}

void InterpreterCodeImpl::doCompare(TokenKind op)
{
    int64_t u = popFromStack();
    int64_t l = popFromStack();
    int16_t offset = getNext<int16_t>();
    bool cond = false;
    switch (op) {
        case tEQ: cond = (u == l);
            break;
        case tNEQ: cond = (u != l);
            break;
        case tGT: cond = (u > l);
            break;
        case tGE: cond = (u >= l);
            break;
        case tLT: cond = (u < l);
            break;
        case tLE: cond = (u <= l);
            break;
        default:
            break;
    }
    if (cond)
        m_pc += offset - 2;
}

void InterpreterCodeImpl::doCall(uint16_t funId, BytecodeFunction *fun)
{
      if (fun == 0)
        fun = (BytecodeFunction *)functionById(funId);
      CallFrame callFrame = {m_pc, m_funId, m_currentCtx, *(m_fun_map_start + funId)};
      m_callStack.push_back(callFrame);
      m_funId = funId;
      m_bytecode = fun->bytecode();
      m_currentCtx = m_currentCtx + m_currentLocalNum;
      m_currentLocalNum = fun->localsNumber();
      m_pc = 0;
      *(m_fun_map_start + funId) = m_currentCtx;
      uint32_t paramsNum = fun->parametersNumber();
      for (uint32_t i = 0; i < paramsNum; i++) {
           *(m_currentCtx + i) = *(m_tos - paramsNum + i);
      }
      m_tos -= paramsNum;
}

void InterpreterCodeImpl::doCallNative(uint16_t funId)
{
    error("Interpreter : not supported");
}

void InterpreterCodeImpl::doReturn()
{
    CallFrame& callFrame = m_callStack.back();
    m_callStack.pop_back();
    BytecodeFunction *fun = (BytecodeFunction *)functionById(callFrame.funId);
    *(m_fun_map_start + m_funId) = callFrame.prevCallFunCtx;
    m_bytecode = fun->bytecode();
    m_currentLocalNum = m_currentCtx - callFrame.ctx;
    m_funId = callFrame.funId;
    m_currentCtx = callFrame.ctx;
    m_pc = callFrame.pc;

}

void InterpreterCodeImpl::startExec()
{
    static void* dispatch_table[] = {
    #define ENUM_ELEM(b, d, l) &&DO_##b,
        FOR_BYTECODES(ENUM_ELEM)
    #undef ENUM_ELEM
    };

    #define DISPATCH() goto *dispatch_table[m_bytecode->get(m_pc++)]
    BytecodeFunction *main = (BytecodeFunction *)functionByName(AstFunction::top_name);
    doCall(main->id(), main);
    DISPATCH();

    DO_INVALID: error("Interpreter : invalid command");
    DO_DLOAD: pushToStack(getNext<double>()); DISPATCH();
    DO_ILOAD: pushToStack(getNext<int64_t>()); DISPATCH();
    DO_SLOAD: pushToStack(nextString()); DISPATCH();
    DO_DLOAD0: pushToStack(0.); DISPATCH();
    DO_ILOAD0: pushToStack(0LL); DISPATCH();
    DO_SLOAD0: pushToStack(""); DISPATCH();
    DO_DLOAD1: pushToStack(1.); DISPATCH();
    DO_ILOAD1: pushToStack(1LL); DISPATCH();
    DO_DLOADM1: pushToStack(-1.); DISPATCH();
    DO_ILOADM1: pushToStack(-1LL); DISPATCH();
    DO_DADD: doBinaryOp<double>(tADD); DISPATCH();
    DO_IADD: doBinaryOp<int64_t>(tADD); DISPATCH();
    DO_DSUB: doBinaryOp<double>(tSUB); DISPATCH();
    DO_ISUB: doBinaryOp<int64_t>(tSUB); DISPATCH();
    DO_DMUL: doBinaryOp<double>(tMUL); DISPATCH();
    DO_IMUL: doBinaryOp<int64_t>(tMUL); DISPATCH();
    DO_DDIV: doBinaryOp<double>(tDIV); DISPATCH();
    DO_IDIV: doBinaryOp<int64_t>(tDIV); DISPATCH();
    DO_IMOD: doIntBinaryOp(tMOD); DISPATCH();
    DO_DNEG: pushToStack(-(double)popFromStack()); DISPATCH();
    DO_INEG: pushToStack(-(int64_t)popFromStack()); DISPATCH();
    DO_IAOR: doIntBinaryOp(tAOR); DISPATCH();
    DO_IAAND: doIntBinaryOp(tAAND); DISPATCH();
    DO_IAXOR: doIntBinaryOp(tAXOR); DISPATCH();
    DO_IPRINT: doPrint<int64_t>(); DISPATCH();
    DO_DPRINT: doPrint<double>(); DISPATCH();
    DO_SPRINT: doPrint<const char*>(); DISPATCH();
    DO_I2D: doConverse(BC_I2D); DISPATCH();
    DO_D2I: doConverse(BC_D2I); DISPATCH();
    DO_S2I: error("Interpreter : not supported");
    DO_SWAP: std::swap(*(m_tos - 2), *(m_tos - 1)); DISPATCH();
    DO_POP: m_tos--; DISPATCH();
    DO_LOADDVAR0:
    DO_LOADIVAR0:
    DO_LOADSVAR0: loadLocalVar(0); DISPATCH();
    DO_LOADDVAR1:
    DO_LOADIVAR1:
    DO_LOADSVAR1: loadLocalVar(1); DISPATCH();
    DO_LOADDVAR2:
    DO_LOADIVAR2:
    DO_LOADSVAR2: loadLocalVar(2); DISPATCH();
    DO_LOADDVAR3:
    DO_LOADIVAR3:
    DO_LOADSVAR3: loadLocalVar(3); DISPATCH();
    DO_STOREDVAR0:
    DO_STOREIVAR0:
    DO_STORESVAR0: storeLocalVar(0); DISPATCH();
    DO_STOREDVAR1:
    DO_STOREIVAR1:
    DO_STORESVAR1: storeLocalVar(1); DISPATCH();
    DO_STOREDVAR2:
    DO_STOREIVAR2:
    DO_STORESVAR2: storeLocalVar(2); DISPATCH();
    DO_STOREDVAR3:
    DO_STOREIVAR3:
    DO_STORESVAR3: storeLocalVar(3); DISPATCH();
    DO_LOADDVAR:
    DO_LOADIVAR:
    DO_LOADSVAR: loadLocalVar(getNext<uint16_t>()); DISPATCH();
    DO_STOREDVAR:
    DO_STOREIVAR:
    DO_STORESVAR: storeLocalVar(getNext<uint16_t>()); DISPATCH();
    DO_LOADCTXDVAR:
    DO_LOADCTXIVAR:
    DO_LOADCTXSVAR: loadContextVar(); DISPATCH();
    DO_STORECTXDVAR:
    DO_STORECTXIVAR:
    DO_STORECTXSVAR: storeContextVar(); DISPATCH();
    DO_DCMP: cmp<double>(); DISPATCH();
    DO_ICMP: cmp<int64_t>(); DISPATCH();
    DO_JA: m_pc += m_bytecode->getInt16(m_pc); DISPATCH();
    DO_IFICMPNE: doCompare(tNEQ); DISPATCH();
    DO_IFICMPE: doCompare(tEQ); DISPATCH();
    DO_IFICMPG: doCompare(tGT); DISPATCH();
    DO_IFICMPGE: doCompare(tGE); DISPATCH();
    DO_IFICMPL: doCompare(tLT); DISPATCH();
    DO_IFICMPLE: doCompare(tLE); DISPATCH();
    DO_DUMP: dumpTos(); DISPATCH();
    DO_CALL: doCall(getNext<uint16_t>()); DISPATCH();
    DO_CALLNATIVE: doCallNative(getNext<uint16_t>()); DISPATCH();
    DO_RETURN: doReturn(); DISPATCH();
    DO_BREAK: error("Interpreter : not supported");
    DO_STOP: return;
}



Status *InterpreterCodeImpl::execute(vector<mathvm::Var *> &vars)
{
    assert(vars.size() == 0);
    try {
        FunctionIterator it = Code::FunctionIterator(this);
        uint16_t funNum = 0;
        while (it.hasNext()) {
            it.next();
            funNum++;
        }
        m_funIdToCtx.reserve(funNum);
        m_fun_map_start = &m_funIdToCtx[0];
        startExec();
    } catch (ErrorInfoHolder* error) {
        return Status::Error(error->getMessage(), error->getPosition());
    }
    return Status::Ok();
}


void InterpreterCodeImpl::error(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    verror(0, format, args);
}

void StackValue::doConverse(Instruction conv)
{
    if (conv == BC_I2D)
        doubleValue = intValue;
    else if (conv == BC_D2I)
        intValue = doubleValue;
    else
        assert(false);
}

}
