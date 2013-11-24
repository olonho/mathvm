#include <cstdlib>
#include <iostream>
#include <fstream>

#include "bytecode_translator.h"

using namespace mathvm;

void usage(char const* prog) {
   std::cerr << "USAGE: " << prog << " input-file" << std::endl;
   std::cerr << " input-file - source code for mvm language" << std::endl;
   std::cerr << " Program will interpret code" << std::endl;
}

void get_source_code(char const* filename, std::string& source_code) {
   std::string buf;
   std::ifstream input(filename);
   while(std::getline(input, buf)) {
      source_code.append(buf);
      source_code.push_back('\n');
   }
}

void get_expected_result(std::string const& code_filename, std::string& res) {
   std::string const& res_filename =
      code_filename.substr(0, code_filename.find_last_of('.')) + ".expect";
   std::string buf;
   std::ifstream input(res_filename.c_str());
   while(std::getline(input, buf)) {
      res.append(buf);
      res.push_back('\n');
   }
}

int main(int argc, char** argv) {
   if(argc != 2) {
      usage(argv[0]);
      exit(1);
   }

   std::string source_code;
   get_source_code(argv[1], source_code);
   Translator* translator = new BytecodeTranslatorImpl();
   //std::cout << "--- ORIGINAL ---" << std::endl;
   //std::cout << source_code << std::endl;

   Code* code;
   if(Status* s = translator->translate(source_code, &code)) {
      std::cerr << "TRANSLATOR ERROR: " << s->getError()
                << " @ " << s->getPosition() << std::endl;
      delete s;
      exit(1);
   }
   //std::cout << "--- BYTECODE ---" << std::endl;
   //code->disassemble(std::cout);

   //std::string expected_result;
   //get_expected_result(argv[1], expected_result);
   //std::cout << "--- EXPECTED ---" << std::endl;
   //std::cout << expected_result;

   //std::cout << "--- EXECUTION ---" << std::endl;
   vector<Var*> vars;
   if(Status* s = code->execute(vars)) {
      std::cerr << "INTERPRETER ERROR: " << s->getError()
                << " @ " << s->getPosition() << std::endl;
      delete s;
      exit(1);
   }
   delete translator;
   delete code;
   return 0;
}
