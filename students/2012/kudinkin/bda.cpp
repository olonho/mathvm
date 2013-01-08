
#include "bda.h"


namespace mathvm
{
  
  stringstream disassemble(Code *code)
  {

    // Create BC-stream
    stringstream bcstream;
    
    for (Code::FunctionIterator fi(code); fi.hasNext();)
    {
      BytecodeFunction *f = static_cast<BytecodeFunction*>(fi.next());
      
      bcstream << f->name() << ":\n";
      
      // Retrieve BC-section
      Bytecode *bcs = f->bytecode();
      
      for (uint16_t i = 0; i < bcs->length();)
      {
        Instruction next;
        switch (bcs->getInsn(i)) {
#ifdef  GEN_CASE_ST_FOR_BYTECODE
#undef  GEN_CASE_ST_FOR_BYTECODE
#endif

#define _fstr(x) #x
#define str(x) _fstr(x)

#define GEN_CASE_ST_FOR_BYTECODE(b, d, l) \
  case (BC_##b): bcstream << i << ":\t" << str(BC_##b); next = BC_##b; break;
        
          FOR_BYTECODES(GEN_CASE_ST_FOR_BYTECODE)
        
#undef  GEN_CASE_ST_FOR_BYTECODE
        }
        
        // ...
        
        ++i;
        
        switch(next) {
          
          case BC_DLOAD:
            bcstream << " @" << bcs->getTyped<double>(i);
            i += sizeof(double);
            break;
          
          case BC_ILOAD:
            bcstream << " @" << bcs->getInt64(i);
            i += sizeof(int64_t);
            break;
            
          
          case BC_SLOAD:
          
          case BC_LOADDVAR:
          case BC_LOADIVAR:
          case BC_LOADSVAR:
          
          case BC_STOREDVAR:
          case BC_STOREIVAR:
          case BC_STORESVAR:

          case BC_JA:
          case BC_IFICMPE:
          case BC_IFICMPNE:
          case BC_IFICMPG:
          case BC_IFICMPGE:
          case BC_IFICMPL:
          case BC_IFICMPLE:
        
          case BC_CALL:
          case BC_CALLNATIVE:
            
            bcstream << " :" << bcs->getInt16(i);
            i += sizeof(int16_t);
            break;

          case BC_LOADCTXDVAR:
          case BC_LOADCTXIVAR:
          case BC_LOADCTXSVAR:
          case BC_STORECTXDVAR:
          case BC_STORECTXIVAR:
          case BC_STORECTXSVAR:
            
            bcstream << " :" << bcs->getUInt16(i);
            i += sizeof(uint16_t);
            bcstream << ":" << bcs->getUInt16(i);
            i += sizeof(uint16_t);
            break;
            
        }
        
        bcstream << endl;
        
      }
    }
    
    return bcstream;
    
  }
  
} // namespace mathvm