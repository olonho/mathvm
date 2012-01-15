#include "MyInterpreter.h"
#include "GeneratorCommon.h"
#include <sstream>

using namespace mathvm;
using namespace std;

Status* Interpeter::execute( std::vector<mathvm::Var*>& vars )
{
  myCurrentFrame = NULL;
  
  this->AllocateFrameStack(50*1024);
  AllocateFrame(0);

  while (true) {
    Instruction ins = GetNextInstruction();
    switch (ins) {
    
    //-LOAD----------------------------------------------------------------
    case BC_ILOAD: 
      Push(Next<int64_t>()); break;
    case BC_ILOAD0:
      Push((int64_t)0); break;
    case BC_ILOAD1:
      Push((int64_t)1); break;
    case BC_ILOADM1:
      Push((int64_t)-1); break;
    case BC_DLOAD:
      Push(Next<double>()); break;
    case BC_DLOAD0:
      Push((double)0); break;
    case BC_DLOAD1:
      Push((double)1); break;
    case BC_DLOADM1:
      Push((double)-1); break;
    case BC_SLOAD:
      PushString(Next<uint16_t>()); break;

    //-LocalVariables------------------------------------------------------
    case BC_STOREIVAR:
      {
        uint16_t id = Next<uint16_t>();
        myCurrentFrame->vars[id].i = PopInt();
        break;
      }
    case BC_STOREDVAR:
      {
        uint16_t id = Next<uint16_t>();
        myCurrentFrame->vars[id].d = PopDouble();
        break;
      }
    case BC_STORESVAR:
      {
        uint16_t id = Next<uint16_t>();
        myCurrentFrame->vars[id].s = PopString();
        break;
      }
      
    case BC_LOADIVAR:
    case BC_LOADDVAR:
    case BC_LOADSVAR:
      {
        uint16_t id = Next<uint16_t>();
        Push(myCurrentFrame->vars[id]);
        break;
      }

    //-ContextVariables----------------------------------------------------
    case BC_STORECTXIVAR:
      {
        uint16_t frameId = Next<uint16_t>();
        uint16_t id = Next<uint16_t>();
        StackFrame* frame = FindFrame(frameId);
        frame->vars[id].i = PopInt();
        break;
      }
    case BC_STORECTXDVAR:
      {
        uint16_t frameId = Next<uint16_t>();
        uint16_t id = Next<uint16_t>();
        StackFrame* frame = FindFrame(frameId);
        frame->vars[id].d = PopDouble();
        break;
      }
    case BC_STORECTXSVAR:
      {
        uint16_t frameId = Next<uint16_t>();
        uint16_t id = Next<uint16_t>();
        StackFrame* frame = FindFrame(frameId);
        frame->vars[id].s = PopString();
        break;
      }
    case BC_LOADCTXSVAR:
    case BC_LOADCTXDVAR:
    case BC_LOADCTXIVAR:
      {
        uint16_t frameId = Next<uint16_t>();
        uint16_t id = Next<uint16_t>();
        StackFrame* frame = FindFrame(frameId);
        Push(frame->vars[id]);
      }
      break;

    //-Arithmetics---------------------------------------------------------
    case BC_IADD:
      Push(PopInt() + PopInt()); break;
    case BC_DADD:
      Push(PopDouble() + PopDouble()); break;
    case BC_IMUL:
      Push(PopInt() * PopInt()); break;
    case BC_DMUL:
      Push(PopDouble() * PopDouble()); break;
    case BC_ISUB: 
      {
        int64_t one = PopInt();
        int64_t two = PopInt();
        Push(two - one);
      }
      break;
    case BC_DSUB: 
      {
        double one = PopDouble();
        double two = PopDouble();
        Push(two - one);
      }
      break;
    case BC_IDIV:
      {
        int64_t one = PopInt();
        int64_t two = PopInt();
        Push(two / one);
      }
      break;
    case BC_DDIV:
      {
        double one = PopDouble();
        double two = PopDouble();
        Push(two / one);
      }
      break;
    case BC_INEG:
      Push(-PopInt()); break;
    case BC_DNEG:
      Push(-PopDouble()); break;

    //-Conversions---------------------------------------------------------
    case BC_I2D:
      myVariablesStack[myVariablesStackIP].d = (double)myVariablesStack[myVariablesStackIP].i; break; 
    case BC_D2I:
      myVariablesStack[myVariablesStackIP].i = (int)myVariablesStack[myVariablesStackIP].d; break;
    
    //-Printing------------------------------------------------------------
    case BC_IPRINT:
      Print(PopInt()); break;
    case BC_DPRINT:
      Print(PopDouble()); break;
    case BC_SPRINT:
      Print(PopString()); break;

    //-Functions-----------------------------------------------------------
    case BC_CALL: 
      AllocateFrame(Next<uint16_t>()); break;
    case BC_RETURN:
      PopCurrentFrame(); break;

    //-Jumps---------------------------------------------------------------
    case BC_JA:
      Jump(Next<int16_t>()); break;

    case BC_IFICMPNE:
      {
        int64_t lower = PopInt();
        int64_t upper = PopInt();
        int16_t ja = Next<int16_t>();
        if (upper != lower) Jump(ja);
      }
      break;
    case BC_IFICMPE:
      {
        int64_t lower = PopInt();
        int64_t upper = PopInt();
        int16_t ja = Next<int16_t>();
        if (upper == lower) Jump(ja);
      }
      break;
    case BC_IFICMPG:
      {
        int64_t lower = PopInt();
        int64_t upper = PopInt();
        int16_t ja = Next<int16_t>();
        if (upper > lower) Jump(ja);
      }
      break;
    case BC_IFICMPGE:
      {
        int64_t lower = PopInt();
        int64_t upper = PopInt();
        int16_t ja = Next<int16_t>();
        if (upper >= lower) Jump(ja);
      }
      break;
    case BC_IFICMPL:
      {
        int64_t lower = PopInt();
        int64_t upper = PopInt();
        int16_t ja = Next<int16_t>();
        if (upper < lower) Jump(ja);
      }
      break;
    case BC_IFICMPLE:
      {
        int64_t lower = PopInt();
        int64_t upper = PopInt();
        int16_t ja = Next<int16_t>();

        if (upper <= lower) Jump(ja);
      }
      break;
    //-Other---------------------------------------------------------------

    case BC_DCMP:
      {
        double upper = PopDouble();
        double lower = PopDouble();
        int64_t result = 0;
        if (upper < lower) result = -1;
        else if (upper > lower) result = 1;
        Push(result);
      }

    case BC_STOP:
      PopCurrentFrame();
      return NULL;

    default:
      throw InterpretationException("Unknown command");
    }
  }

  return NULL;
}

mathvm::Instruction Interpeter::GetNextInstruction()
{
  Instruction i = myCurrentBytecode->getInsn(*myIP);
  ++(*myIP);
  return i;
}

Interpeter::Interpeter() : myIP(0), myCurrentBytecode(NULL)
{

}

void Interpeter::Push( int64_t value )
{
  myVariablesStack[++myVariablesStackIP].i = value;
}

void Interpeter::Push( double value )
{
  myVariablesStack[++myVariablesStackIP].d = value;
}

void Interpeter::Push( StackVariable const & var )
{
  myVariablesStack[++myVariablesStackIP] = var;
}

void Interpeter::PushString( uint16_t id )
{
  myVariablesStack[++myVariablesStackIP].s = this->constantById(id).c_str();
}

int64_t Interpeter::PopInt()
{
  return myVariablesStack[myVariablesStackIP--].i;
}

double Interpeter::PopDouble()
{
  return myVariablesStack[myVariablesStackIP--].d;
}

char const * Interpeter::PopString()
{
  return myVariablesStack[myVariablesStackIP--].s;
}

StackFrame* Interpeter::AllocateFrame(uint16_t functionId )
{
  BytecodeFunction * fun = (BytecodeFunction*) this->functionById(functionId);
  StackFrame * frame = AddFrame(fun->localsNumber(), functionId);
  frame->prevFrame = myCurrentFrame;
  if (myCurrentFrame) {
    if (myCurrentFrame->functionId != functionId) frame->prevDifferentFrame = myCurrentFrame;
    else frame->prevDifferentFrame = myCurrentFrame->prevDifferentFrame;
  } 
  myIP = &frame->ip;
  assert(fun);
  myCurrentBytecode = fun->bytecode();
  myCurrentFrame = frame;

  for (unsigned int i = fun->parametersNumber(); i > 0; --i) {
    switch (fun->parameterType(i-1)) {
    case VT_INT: myCurrentFrame->vars[i-1].i = PopInt(); break;
    case VT_DOUBLE: myCurrentFrame->vars[i-1].d = PopDouble(); break;
    case VT_STRING: myCurrentFrame->vars[i-1].s = PopString(); break;
		default: throw InterpretationException("Just to shut up annoying GCC warning...");
    }
  }

  return frame;
}

void Interpeter::PopCurrentFrame()
{
  if (myCurrentFrame->prevFrame) {
    myIP = &myCurrentFrame->prevFrame->ip;
    BytecodeFunction * fun = (BytecodeFunction*)this->functionById(myCurrentFrame->prevFrame->functionId);
    myCurrentBytecode = fun->bytecode();
  }
  myFrameStackPoolIP -= myCurrentFrame->size;
  myCurrentFrame = myCurrentFrame->prevFrame;
}

void Interpeter::Jump( int16_t offset )
{
  // 2 - is the size of jump address
  *myIP = *myIP - 2 + offset;
}

StackFrame* Interpeter::FindFrame( uint16_t frameId )
{
  if (frameId == 0) return (StackFrame*)myFrameStackPool;
  StackFrame* frame = myCurrentFrame;
  while (frame) {
    if (frame->functionId == frameId) {
      return frame;
    }
    frame = frame->prevDifferentFrame;
  }
  throw InterpretationException("Frame not found");
}

void Interpeter::Print( int64_t value )
{
  std::cout << value;
}

void Interpeter::Print( double value )
{
	std::stringstream stream;
	stream << value;
	std::string s = stream.str();
	int i = s.find("e+0");
	if (i != -1) {
		s.replace(i, 3, "e+");
	}

	i = s.find("e-0");
	if (i != -1) {
		s.replace(i, 3, "e-");
	}

  std::cout << s;
}

void Interpeter::Print( char const * value )
{
  std::cout << value;
}

void Interpeter::AllocateFrameStack( int stackSizeInKb )
{
  myFrameStackPoolSize = stackSizeInKb * 1024;
  myFrameStackPool = new char[myFrameStackPoolSize];
  myFrameStackPoolIP = 0;

  myVariablesStack = new StackVariable[1024];
  myVariablesStackIP = -1;
}

StackFrame * Interpeter::AddFrame( uint16_t localsNumber, uint16_t functionId )
{
  StackFrame * result = new (&myFrameStackPool[myFrameStackPoolIP]) StackFrame(localsNumber, functionId);
  myFrameStackPoolIP += result->size;
  return result;
}


