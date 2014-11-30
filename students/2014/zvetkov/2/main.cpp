#include "bytecode_generator.hpp"
#include "bytecode_interpreter.hpp"
#include "errors.hpp"
#include "mathvm.h"

#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <sys/stat.h>

#include <exception>
#include <iostream>
#include <string>

using namespace mathvm;
using namespace std;

Translator* Translator::create(const string& impl) {
  if (impl == "bytecode_translator") {
    return new BytecodeTranslatorImpl();
  } 

  return 0;
}

int main(int argc, char** argv) {
  string program;

  for (int i = 0; i < argc; ++i) {
    string arg = argv[i];

    if (arg == "-e" && i + 1 < argc) {
        program = argv[++i];
        continue;
    }

    program = loadFile(arg.c_str());
  }

  if (program.empty()) { 
    cerr << "Could not load program\n"
    << "Usage:\n"
    << "mvm PATH_TO_SOURCE\n"
    << "mvm -e SCRIPT" << endl; 
    return EXIT_FAILURE;
  }    

  Translator* translator = Translator::create("bytecode_translator");

  if (!translator) { 
    cerr << "Define translator impl at factory" << endl;
    return EXIT_FAILURE;
  }

  Code* code = 0;
  Status* translateStatus;
  
  try {
    translateStatus = translator->translate(program, &code);
  } catch (TranslationException& e) {
    cerr << errorMessage(program.c_str(), e.what(), e.position()) << endl;
    return EXIT_FAILURE;
  } 

  if (translateStatus->isError()) {
    cerr << errorMessage(program.c_str(), translateStatus) << endl;
    return EXIT_FAILURE;
  }

  try {
    BytecodeInterpreter vm(code);
    vm.execute();
  } catch (InterpreterException& e) {
    cerr << e.what() << endl;
    return EXIT_FAILURE;
  } 

  if (code) {
    delete code;
  }

  delete translator;
  delete translateStatus;

  return 0;
}