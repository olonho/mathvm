#include "../../../../include/mathvm.h"
#include "function.h" 
#include "hash_table.h"
#include "cinterpreter.h" 
#include "Translator.h"

using namespace mathvm;
using namespace std; 

int main(int argc, char** argv) {
   
  if(argc != 2) {
    cout << "Usage: " << argv[0] << " <input>" << endl;
    return 1;
  }
  
  char const * program = loadFile(argv[1]);
  if (program == NULL) {
    cout << "Can't read file " << argv[1] << endl;
    return 1;
  }
  
  Code * code;
  BytecodeTranslatorImpl translator;
  
  Status * status = translator.translate(program, &code);
  if (status->isError()) {
    cout << status->getError() << endl;
  } else {
    // code->disassemble();
    delete status; 
    interpreter_start( table_get( 0 )-> code ); 
  }
  
  
  // delete status; 
  delete [] program;
  
  return 0;
}
