#include "MyInterpreter.h"

using namespace mathvm;
using namespace std;

Status* MyInterpeter::execute(std::vector<mathvm::Var*>& vars)
{
    allocateFrame(0);

    while (true) {
        Instruction ins = getNextInstruction();
        switch (ins) {

        //-LOAD----------------------------------------------------------------
        case BC_ILOAD:
            push(Next<int64_t>()); break;
        case BC_ILOAD0:
            push((int64_t)0); break;
        case BC_ILOAD1:
            push((int64_t)1); break;
        case BC_ILOADM1:
            push((int64_t)-1); break;
        case BC_DLOAD:
            push(Next<double>()); break;
        case BC_DLOAD0:
            push((double)0); break;
        case BC_DLOAD1:
            push((double)1); break;
        case BC_DLOADM1:
            push((double)-1); break;
        case BC_SLOAD:
            pushString(Next<uint16_t>()); break;

            //-LocalVariables------------------------------------------------------
        case BC_STOREIVAR:
        {
            uint16_t id = Next<uint16_t>();
            myCurrentFrame->vars[id].i = popInt();
            break;
        }
        case BC_STOREDVAR:
        {
            uint16_t id = Next<uint16_t>();
            myCurrentFrame->vars[id].d = popDouble();
            break;
        }
        case BC_STORESVAR:
        {
            uint16_t id = Next<uint16_t>();
            myCurrentFrame->vars[id].s = popString();
            break;
        }

        case BC_LOADIVAR:
        case BC_LOADDVAR:
        case BC_LOADSVAR:
        {
            uint16_t id = Next<uint16_t>();
            push(myCurrentFrame->vars[id]);
            break;
        }

            //-ContextVariables----------------------------------------------------
        case BC_STORECTXIVAR:
        {
            uint16_t frameId = Next<uint16_t>();
            uint16_t id = Next<uint16_t>();
            StackFrame* frame = findFrame(frameId);
            frame->vars[id].i = popInt();
            break;
        }
        case BC_STORECTXDVAR:
        {
            uint16_t frameId = Next<uint16_t>();
            uint16_t id = Next<uint16_t>();
            StackFrame* frame = findFrame(frameId);
            frame->vars[id].d = popDouble();
            break;
        }
        case BC_STORECTXSVAR:
        {
            uint16_t frameId = Next<uint16_t>();
            uint16_t id = Next<uint16_t>();
            StackFrame* frame = findFrame(frameId);
            frame->vars[id].s = popString();
            break;
        }
        case BC_LOADCTXSVAR:
        case BC_LOADCTXDVAR:
        case BC_LOADCTXIVAR:
        {
            uint16_t frameId = Next<uint16_t>();
            uint16_t id = Next<uint16_t>();
            StackFrame* frame = findFrame(frameId);
            push(frame->vars[id]);
        }
            break;

            //-Arithmetics---------------------------------------------------------
        case BC_IADD:
            push(popInt() + popInt()); break;
        case BC_DADD:
            push(popDouble() + popDouble()); break;
        case BC_IMUL:
            push(popInt() * popInt()); break;
        case BC_DMUL:
            push(popDouble() * popDouble()); break;
        case BC_ISUB:
        {
            int64_t one = popInt();
            int64_t two = popInt();
            push(two - one);
        }
            break;
        case BC_DSUB:
        {
            double one = popDouble();
            double two = popDouble();
            push(two - one);
        }
            break;
        case BC_IDIV:
        {
            int64_t one = popInt();
            int64_t two = popInt();
            push(two / one);
        }
            break;
        case BC_DDIV:
        {
            double one = popDouble();
            double two = popDouble();
            push(two / one);
        }
            break;
        case BC_INEG:
            push(-popInt()); break;
        case BC_DNEG:
            push(-popDouble()); break;

            //-Conversions---------------------------------------------------------
        case BC_I2D:
            myStack.top().d = (double)myStack.top().i; break;
        case BC_D2I:
            myStack.top().i = (int)myStack.top().d; break;

            //-printing------------------------------------------------------------
        case BC_IPRINT:
            print(popInt()); break;
        case BC_DPRINT:
            print(popDouble()); break;
        case BC_SPRINT:
            print(popString()); break;

            //-Functions-----------------------------------------------------------
        case BC_CALL:
            allocateFrame(Next<uint16_t>()); break;
        case BC_RETURN:
            popCurrentFrame(); break;

            //-jumps---------------------------------------------------------------
        case BC_JA:
            jump(Next<int16_t>()); break;

        case BC_IFICMPNE:
        {
            int64_t lower = popInt();
            int64_t upper = popInt();
            int16_t ja = Next<int16_t>();
            if (upper != lower) jump(ja);
        }
            break;
        case BC_IFICMPE:
        {
            int64_t lower = popInt();
            int64_t upper = popInt();
            int16_t ja = Next<int16_t>();
            if (upper == lower) jump(ja);
        }
            break;
        case BC_IFICMPG:
        {
            int64_t lower = popInt();
            int64_t upper = popInt();
            int16_t ja = Next<int16_t>();
            if (upper > lower) jump(ja);
        }
            break;
        case BC_IFICMPGE:
        {
            int64_t lower = popInt();
            int64_t upper = popInt();
            int16_t ja = Next<int16_t>();
            if (upper >= lower) jump(ja);
        }
            break;
        case BC_IFICMPL:
        {
            int64_t lower = popInt();
            int64_t upper = popInt();
            int16_t ja = Next<int16_t>();
            if (upper < lower) jump(ja);
        }
            break;
        case BC_IFICMPLE:
        {
            int64_t lower = popInt();
            int64_t upper = popInt();
            int16_t ja = Next<int16_t>();

            if (upper <= lower) jump(ja);
        }
            break;
            //-Other---------------------------------------------------------------

        case BC_DCMP:
        {
            double upper = popDouble();
            double lower = popDouble();
            int64_t result = 0;
            if (upper < lower) result = -1;
            else if (upper > lower) result = 1;
            push(result);
        }

        case BC_STOP:
            popCurrentFrame();
            return NULL;

        default:
            throw InterpretationException("Unknown command");
        }

    }

    return NULL;
}

mathvm::Instruction MyInterpeter::getNextInstruction() {
    Instruction i = myCurrentBytecode->getInsn(*myIP);
    ++(*myIP);
    return i;
}

MyInterpeter::MyInterpeter() : myIP(0), myCurrentBytecode(NULL) {
}

void MyInterpeter::push(int64_t value) {
    StackVariable v;
    v.i = value;
    myStack.push(v);
}

void MyInterpeter::push(double value) {
    StackVariable v;
    v.d = value;
    myStack.push(v);
}

void MyInterpeter::push(StackVariable const & var) {
    myStack.push(var);
}

void MyInterpeter::pushString(uint16_t id) {
    StackVariable v;
    v.s = this->constantById(id).c_str();
    myStack.push(v);
}

int64_t MyInterpeter::popInt() {
    int64_t result = myStack.top().i;
    myStack.pop();
    return result;
}

double MyInterpeter::popDouble() {
    double result = myStack.top().d;
    myStack.pop();
    return result;
}

char const * MyInterpeter::popString() {
    char const * result = myStack.top().s;
    myStack.pop();
    return result;
}

StackFrame* MyInterpeter::allocateFrame(uint16_t functionId) {
    ExtendedBytecodeFunction * fun = (ExtendedBytecodeFunction*) this->functionById(functionId);
    StackFrame * frame = new StackFrame(fun->GetVariablesNum(), functionId);
    if (!myFrameStack.empty()) frame->prevFrame = myFrameStack.top();
    myIP = &frame->ip;
    assert(fun);
    myCurrentBytecode = fun->bytecode();
    myFrameStack.push(frame);
    myCurrentFrame = frame;

    std::vector<mathvm::VarType> args = fun->getArgumentTypes();
    for (unsigned int i = args.size(); i > 0 ; --i) {
        switch (args[i-1]) {
        case VT_INT: myCurrentFrame->vars[i-1].i = popInt();
            break;
        case VT_DOUBLE: myCurrentFrame->vars[i-1].d = popDouble();
            break;
        case VT_STRING: myCurrentFrame->vars[i-1].s = popString();
            break;
        default: throw InterpretationException("Invalid argument type");
        }
    }
    return frame;
}

void MyInterpeter::popCurrentFrame() {
    if (myCurrentFrame->prevFrame) {
        myIP = &myCurrentFrame->prevFrame->ip;
        BytecodeFunction * fun = (BytecodeFunction*)this->functionById(myCurrentFrame->prevFrame->functionId);
        myCurrentBytecode = fun->bytecode();
    }
    StackFrame * temp = myCurrentFrame;
    myCurrentFrame = myCurrentFrame->prevFrame;
    delete temp;
    myFrameStack.pop();
}

void MyInterpeter::jump(int16_t offset) {
    // 2 - is the size of jump address
    *myIP = *myIP - 2 + offset;
}

StackFrame* MyInterpeter::findFrame(uint16_t frameId) {
    static StackFrame * frameCache = 0;
    if (frameCache != 0 && frameCache->functionId == frameId) return frameCache;

    StackFrame* frame = myFrameStack.top();
    do {
        if (frame->functionId == frameId) {
            frameCache = frame;
            return frame;
        }
        frame = frame->prevFrame;
    } while (frame);
    throw InterpretationException("Frame not found");
}

void MyInterpeter::print(int64_t value) {
    std::cout << value;
}

void MyInterpeter::print(double value) {
    std::cout << value;
}

void MyInterpeter::print(char const * value) {
    std::cout << value;
}


