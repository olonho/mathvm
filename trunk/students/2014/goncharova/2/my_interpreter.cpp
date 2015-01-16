#include "my_interpreter.h"
#include <stdexcept>
#include <elf.h>

template<>
double StackUnit::getValue<double>() const {
    return doubleVal;
}
template<>
int64_t StackUnit::getValue<int64_t>() const {
    return intVal;
}
template<>
char const * StackUnit::getValue<char const *>() const {
    return stringVal;
};

#define BINOP(OP, TYPE) {                                      \
  StackUnit arg1 = popFromStack();                             \
  StackUnit arg2 = popFromStack();                             \
  pushOnStack(arg2.getValue<TYPE>() OP arg1.getValue<TYPE>()); \
  break;                                                       \
}

int64_t parseInt(char const * string) {
    char *p;
    int64_t result = strtoll(string, &p, 10);
    if ('\0' != *p) {
        throw std::runtime_error("S2I failed.");
    }
    return result;
}

void BytecodeContext::jump(offsetType offset) {
    ip += offset;
}

bool BytecodeContext::ipIsValid() {
    return ip < byteCode->length();
}

void BytecodeInterpreter::jump() {
    offsetType offset = currentContext().nextOffset();
    currentContext().jump(offset - sizeof(offset));
}
void BytecodeInterpreter::jumpCond(bool cond) {
    if (cond) {
        jump();
    } else {
        currentContext().nextId();
    };
}

void BytecodeInterpreter::call(idType funId) {
    TranslatedFunction* tFn = code->functionById(funId);
    BytecodeFunction* bFn = (BytecodeFunction*) tFn;

    contexts.push_back(BytecodeContext(bFn));
}

void BytecodeInterpreter::_return() {
    contexts.pop_back();
}


void BytecodeInterpreter::interpretInstructions() {
    while (canInterpret()) {
        Instruction instruction = nextInstruction();
//        std::cout << "executing instruction " << bytecodeName(instruction, 0) << std::endl;
//        std::cout.flush();
        switch (instruction) {
            case BC_DLOAD:
                pushOnStack(currentContext().nextDouble());
                break;
            case BC_ILOAD:
                pushOnStack(currentContext().nextInt());
                break;
            case BC_SLOAD:
                pushOnStack(code->constantById(currentContext().nextId()).c_str());
                break;
            case BC_DLOAD0:
                pushOnStack(0.0);
                break;
            case BC_ILOAD0:
                pushOnStack((int64_t)0);
                break;
            case BC_DLOAD1:
                pushOnStack(1.0);
                break;
            case BC_ILOAD1:
                pushOnStack((int64_t)1);
                break;
            case BC_DLOADM1:
                pushOnStack(-1.0);
                break;
            case BC_ILOADM1:
                pushOnStack((int64_t)-1);
                break;
            case BC_DADD: BINOP(+, double)
            case BC_IADD: BINOP(+, int64_t)
            case BC_DSUB: BINOP(-, double)
            case BC_ISUB: BINOP(-, int64_t)
            case BC_DMUL: BINOP(*, double)
            case BC_IMUL: BINOP(*, int64_t)
            case BC_DDIV: BINOP(/, double)
            case BC_IDIV: BINOP(/, int64_t)
            case BC_IMOD: BINOP(%, int64_t)
            case BC_DNEG: {
                double value = popFromStack().getDouble();
                pushOnStack((double) (value == 0 ? 1 : 0));
            }
                break;
            case BC_INEG: {
                int64_t intVal = popFromStack().getInt();
                pushOnStack((int64_t) (intVal == 0 ? 1 : 0));}
                break;
            case BC_IAOR: BINOP(|, int64_t)
            case BC_IAAND: BINOP(&, int64_t)
            case BC_IAXOR: BINOP(^, int64_t)
            case BC_IPRINT:
                std::cout << popFromStack().getInt();
                break;
            case BC_DPRINT:
                std::cout << popFromStack().getDouble();
                break;
            case BC_SPRINT:
            std::cout << popFromStack().getString();
                break;
            case BC_I2D:
                pushOnStack((double) popFromStack().getInt());
                break;
            case BC_D2I:
                pushOnStack((int64_t) popFromStack().getDouble());
                break;
            case BC_S2I:
                pushOnStack(parseInt(popFromStack().getString()));
                break;
            case BC_SWAP: {
                StackUnit top = popFromStack();
                StackUnit bot = popFromStack();
                pushOnStack(top);
                pushOnStack(bot);
            }
                break;
            case BC_POP:
                popFromStack();
                break;
            case BC_LOADDVAR:
                pushOnStack(currentContext().getDoubleVar(currentContext().nextId()));
                break;
            case BC_LOADIVAR:
                pushOnStack(currentContext().getIntVar(currentContext().nextId()));
                break;
            case BC_LOADSVAR:
                pushOnStack(currentContext().getStringVar(currentContext().nextId()));
                break;
            case BC_STOREDVAR:
                currentContext().setVar(currentContext().nextId(), popFromStack().getDouble());
                break;
            case BC_STOREIVAR:
                currentContext().setVar(currentContext().nextId(), popFromStack().getInt());
                break;
            case BC_STORESVAR:
                currentContext().setVar(currentContext().nextId(), popFromStack().getString());
                break;
            case BC_LOADCTXDVAR:
                loadContextVar<double>();
                break;
            case BC_LOADCTXIVAR:
                loadContextVar<int64_t>();
                break;
            case BC_LOADCTXSVAR:
                loadContextVar<char const *>();
                break;
            case BC_STORECTXDVAR:
                storeContextVar<double>();
                break;
            case BC_STORECTXIVAR:
                storeContextVar<int64_t>();
                break;
            case BC_STORECTXSVAR:
                storeContextVar<char const *>();
                break;
            case BC_DCMP:
                compare<double>(true);
                break;
            case BC_ICMP:
                compare<int64_t>(true);
                break;
            case BC_JA:
                jump();
                break;
            case BC_IFICMPNE:
                jumpCond(compare<int64_t>(false) != 0);
                break;
            case BC_IFICMPE:
                jumpCond(compare<int64_t>(false) == 0);
                break;
            case BC_IFICMPG:
                jumpCond(compare<int64_t>(false) < 0);
                break;
            case BC_IFICMPGE:
                jumpCond(compare<int64_t>(false) <= 0);
                break;
            case BC_IFICMPL:
                jumpCond(compare<int64_t>(false) > 0);
                break;
            case BC_IFICMPLE:
                jumpCond(compare<int64_t>(false) >= 0);
                break;
            case BC_CALL:
                call(currentContext().nextId());
                break;
            case BC_RETURN:
                _return();
                break;
            default:
                throw std::runtime_error("unsupported bytecode instruction");

        };
    }
}

bool BytecodeInterpreter::canInterpret() {
    return !contexts.empty() && currentContext().ipIsValid();
}

Instruction BytecodeContext::getInstruction() {
    return byteCode->getInsn(ip++);
}


Instruction BytecodeInterpreter::nextInstruction() {
    return currentContext().getInstruction();

}

BytecodeContext& BytecodeInterpreter::contextById(idType id) {
    for (std::vector<BytecodeContext>::reverse_iterator i = contexts.rbegin(); i != contexts.rend(); ++i) {
        if (i->getId() == id) {
            return *i;
        }
    }
    throw std::runtime_error("Context not found");
}

BytecodeContext& BytecodeInterpreter::currentContext() {
    return contexts.back();
}

void BytecodeContext::setVar(idType varId, int64_t varValue) {
    vars[varId].setInt(varValue);
}

void BytecodeContext::setVar(idType varId, double varValue) {
    vars[varId].setDouble(varValue);
}

void BytecodeContext::setVar(idType varId, char const * varValue) {
    vars[varId].setString(varValue);
}

int64_t BytecodeContext::getIntVar(idType varId) {
    return vars[varId].getInt();
}

double BytecodeContext::getDoubleVar(idType varId) {
    return vars[varId].getDouble();
}

char const * BytecodeContext::getStringVar(idType varId) {
    return vars[varId].getString();
}

int64_t BytecodeContext::nextInt() {
    return nextValue<int64_t>();
}

double BytecodeContext::nextDouble() {
    return nextValue<double>();
}

idType BytecodeContext::nextId() {
    return nextValue<idType>();
}

offsetType BytecodeContext::nextOffset() {
    return nextValue<offsetType>();
}

idType BytecodeContext::getId() {
    return id;
}

BytecodeInterpreter::BytecodeInterpreter(Code *argCode, ostream& stream): code(argCode), outStream(stream) {
}

void BytecodeInterpreter::pushOnStack(StackUnit v) {
    stack.push_back(v);
}

StackUnit BytecodeInterpreter::popFromStack() {
    StackUnit res = stack.back();
    stack.pop_back();
    return res;
}

Status* BytecodeInterpreter::interpret() {
    try {
        call(0);
        interpretInstructions();
        status = Status::Ok();
    } catch (std::runtime_error const & e) {

        status = Status::Error(e.what());
    }
    return status;
}

Status* BytecodeImpl::execute(vector<Var*>& vars) {
    BytecodeInterpreter interpreter(this, std::cout);

    Status* result = interpreter.interpret();
    return result;
}
