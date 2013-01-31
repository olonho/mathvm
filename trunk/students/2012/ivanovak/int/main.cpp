#include <iostream>
#include <string>

#include "bytecode_gen.h"
#include "bc_interpreter.h"

/** To test bc_gen and bc_int use run.py script
*/
int main(int argc, char **argv) {
  /*string cmd;
  
  for(;;) {
    cout << "> ";
    cin >> cmd;
    const char * source = loadFile(cmd.c_str());
    
    if (cmd == "quit") break;
    if (source == 0) {
      cout << "No file found" << endl;
      continue;
    }
  */
  if (argc != 2)
    return 1;
  const char * source = mathvm::loadFile(argv[1]);
  if (!source)
    return 1;
  
  BytecodeInterpretator interpretator;
  BytecodeGenerator generator;
  generator.generate(source, &interpretator);
  std::vector<mathvm::Var*> v;
  interpretator.execute(v);
  //}
  return 0;
}
