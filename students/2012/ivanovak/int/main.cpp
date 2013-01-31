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
  try {
    generator.generate(source, &interpretator);
    std::vector<mathvm::Var*> v;
    interpretator.execute(v);
  } catch(const std::exception& e) {
    std::cerr << "==============================================" << std::endl;
    std::cerr << "Error occured: " << std::endl;
    std::cerr << e.what() << std::endl;
    std::cerr << "==============================================" << std::endl;
    return 1;
  }
  return 0;
}
