#include <fstream>
#include <cstdlib>

#include "bc_interpreter.h"
#include "mathvm.h"

using namespace std;
using namespace mathvm;

struct InvalidBCException : public exception {
   string s;
   InvalidBCException(string ss) : s(ss) {}
   const char* what() const throw() { 
    return s.c_str(); 
   }
   ~InvalidBCException() throw() {}
};

BytecodeInterpretator::BytecodeInterpretator() {
  initLengths();
}

Status* BytecodeInterpretator::execute(vector<Var*> & vars) {
  context = new Context;
  FunctionIterator iter(this);
#if defined(DEBUG) && defined(DEBUG_FILENAME)
  int functions = 0;
#endif
  while (iter.hasNext()) {
    context->addFun((BytecodeFunction*) iter.next());
#if defined(DEBUG) && defined(DEBUG_FILENAME)
  functions++;
#endif
  }
#if defined(DEBUG) && defined(DEBUG_FILENAME)
  fstream filestr;
  filestr.open(DEBUG_FILENAME, fstream::out |  fstream::app);
  filestr << "=======================" << endl;
  filestr << "Functions found: " << functions << endl;
  filestr.close();
#endif
  try {
    callById(0);
  } catch(const InvalidBCException& e) {
    delete context;
    context = 0;
    return new Status(e.what(), 0);
  }
  delete context;
  context = 0;
  return new Status();
}

void BytecodeInterpretator::callById(int id) {
  context->generateStackFrame(id);
  BytecodeFunction * f = context->getFunById(id);
  Bytecode * bytecode = f->bytecode();
  size_t position = 0; 
  double d;
  int64_t i64;
  StackEntry upperse, lowerse;
  int16_t i16;
  double upperd, lowerd;
  int64_t upperi, loweri;
  
  while (position < bytecode->length()) {
    Instruction instr = bytecode->getInsn(position);
    //processInsn(instr, position, fbytecode);
    
    switch (instr) {
    case BC_INVALID:
      throw InvalidBCException("BC Error");
    case BC_DLOAD:
      push<double>(bytecode->getDouble(position + 1));
      break;
    case BC_ILOAD:
      push<int64_t>(bytecode->getInt64(position + 1));
      break;
    case BC_SLOAD:
      push<int16_t>(bytecode->getInt16(position + 1));
      break;
    case BC_DLOAD0:
      push<double>(0);
      break;
    case BC_ILOAD0:
      push<int64_t>(0);
      break;
    case BC_SLOAD0:
      push<int16_t>(0);
      break;
    case BC_DLOAD1:
      push<double>(1);
      break;
    case BC_ILOAD1:
      push<int64_t>(1);
      break;
    case BC_DLOADM1:
      push<double>(-1);
      break;
    case BC_ILOADM1:
      push<int64_t>(-1);
      break;
    case BC_DADD:
      push(pop<double>() + pop<double>());
      break;
    case BC_IADD:
      push(pop<int64_t>() + pop<int64_t>());
      break;
    case BC_DSUB:
      d = pop<double>();
      push(d - pop<double>());
      break;
    case BC_ISUB:
      i64 = pop<int64_t>();
      push(i64 - pop<int64_t>());
      break;
    case BC_DMUL:
      push(pop<double>() * pop<double>());
      break;
    case BC_IMUL:
      push(pop<int64_t>() * pop<int64_t>());
      break;
    case BC_DDIV:
      d = pop<double>();
      push(d / pop<double>());
      break;
    case BC_IDIV:
      i64 = pop<int64_t>();
      push(i64 / pop<int64_t>());
      break;
    case BC_IMOD:
      i64 = pop<int64_t>();
      push(i64 % pop<int64_t>());
      break;
    case BC_DNEG:
      push<double>(-pop<double>());
      break;
    case BC_INEG:
      push<int64_t>(-pop<int64_t>());
      break;
    case BC_DPRINT:
      cout << pop<double>();
      break;
    case BC_IPRINT:
      cout << pop<int64_t>();
      break;
    case BC_SPRINT:
      cout << this->constantById(pop<int16_t>());
      break;
    case BC_D2I:
      d = pop<double>();
      i64 = static_cast<int64_t>(d);
      push(i64);
      break;
    case BC_I2D:
      i64 = pop<int64_t>();
      d = static_cast<double>(i64);
      push(d);
      break;
    case BC_S2I:
    // zero states both for error in conversation and "0" string conversation
#ifdef DEBUG
    //cerr << "String id: " << top<int16_t>() << endl;
#endif
      i64 = atoi(this->constantById(pop<int16_t>()).c_str());
      push<int64_t>(i64);
    break;
    case BC_DCMP:
      upperd = pop<double>();
      lowerd = pop<double>();
      push<int64_t>((upperd < lowerd) ? -1 : (upperd > lowerd));
    break;
    case BC_ICMP:
      upperi = pop<int64_t>();
      loweri = pop<int64_t>();
      push<int64_t>((upperi < loweri) ? -1 : (upperi > loweri));
    break;
    case BC_SWAP:
      upperse = pop<StackEntry>();
      lowerse = pop<StackEntry>();
      push<StackEntry>(upperse);
      push<StackEntry>(lowerse);
      break;
    case BC_POP:
      i64 = pop<int64_t>();
      break;
    case BC_LOADDVAR:
      push<double>(context->getVar<double>(id, 
        bytecode->getInt16(position + 1)));
      break;
    case BC_LOADIVAR:
      push<int64_t>(context->getVar<int64_t>(id, 
        bytecode->getInt16(position + 1)));
      break;
    case BC_LOADSVAR:
      push<int16_t>(context->getVar<int16_t>(id, 
        bytecode->getInt16(position + 1)));
      break;
    case BC_STOREDVAR:
      d = pop<double>();
      context->setVar(id, bytecode->getInt16(position + 1), d);
      break;
    case BC_STOREIVAR:
      i64 = pop<int64_t>();
      context->setVar(id, bytecode->getInt16(position + 1), i64);
      break;
    case BC_STORESVAR:
      i16 = pop<int16_t>();
      context->setVar(id, bytecode->getInt16(position + 1), i16);
      break;
    case BC_LOADCTXIVAR:
      push<int64_t>(context->getVar<int64_t>(
        bytecode->getInt16(position + 1),
        bytecode->getInt16(position + 3)));
      break;
    case BC_STORECTXIVAR:
      i64 = pop<int64_t>();
      context->setVar(bytecode->getInt16(position + 1),
        bytecode->getInt16(position + 3), i64);
      break;
    case BC_LOADCTXDVAR:
      push<double>(context->getVar<double>(
        bytecode->getInt16(position + 1),
        bytecode->getInt16(position + 3)));
      break;
    case BC_STORECTXDVAR:
      d = pop<double>();
      context->setVar(bytecode->getInt16(position + 1),
        bytecode->getInt16(position + 3), d);
      break;
    case BC_LOADCTXSVAR:
      push<int16_t>(context->getVar<int16_t>(
        bytecode->getInt16(position + 1),
        bytecode->getInt16(position + 3)));
      break;
    case BC_STORECTXSVAR:
      i16 = pop<int16_t>();
      context->setVar(bytecode->getInt16(position + 1),
        bytecode->getInt16(position + 3), i16);
      break;
    case BC_CALL:
      callById(bytecode->getInt16(position + 1));
      break;
    case BC_RETURN:
      context->dropStackFrame(id);
      return;
    case BC_JA:
      position += bytecode->getInt16(position + 1) + 1;
      continue;
    case BC_IFICMPNE:
      i64 = pop<int64_t>();
      if (i64 != pop<int64_t>()) {
        position += bytecode->getInt16(position + 1) + 1;
        continue;
      }
      break;
      
    case BC_IFICMPE:
      i64 = pop<int64_t>();
      if (i64 == pop<int64_t>()) {
        position += bytecode->getInt16(position + 1) + 1;
        continue;
      }
      break;
      
    case BC_IFICMPGE:
      i64 = pop<int64_t>();
      if (i64 >= pop<int64_t>()) {
        position += bytecode->getInt16(position + 1) + 1;
        continue;
      }
      break;
      
    case BC_IFICMPG:
      i64 = pop<int64_t>();
      if (i64 > pop<int64_t>()) {
        position += bytecode->getInt16(position + 1) + 1;
        continue;
      }
      break;
      
    case BC_IFICMPLE:
      i64 = pop<int64_t>();
      if (i64 <= pop<int64_t>()) {
        position += bytecode->getInt16(position + 1) + 1;
        continue;
      }
      break;
      
    case BC_IFICMPL:
      i64 = pop<int64_t>();
      if (i64 < pop<int64_t>()) {
        position += bytecode->getInt16(position + 1) + 1;
        continue;
      }
      break;
    case BC_CALLNATIVE:
      // TODO: 
      break;
    
    /** I will use DUMP to convert values on TOS to boolean */
    case BC_DUMP:
      push<int64_t>(pop<int64_t>() == 0 ? 0 : 1);
    default:
      break;
    }

    position += lengths[instr];
  }
  
  context->dropStackFrame(id);
}