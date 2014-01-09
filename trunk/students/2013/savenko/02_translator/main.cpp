#include <iostream>
#include <fstream>
#include <streambuf>
#include <string>
#include <memory>

#include <mathvm.h>

#include <execinfo.h>
#include <signal.h>
#include <cstdlib>

using namespace mathvm;

bool ensureOkStatus(Status * status) {
  bool isOk = true;
  if (status && status->isError()) {
    if (Status::INVALID_POSITION != status->getPosition()) {
      std::cerr << "Error at " << status->getPosition() << ":" << std::endl;
    }
    std::cerr << status->getError() << std::endl;
    isOk = false;
  }
  delete status;
  return isOk;
}

void runTranslator(std::string const & programText) {
  std::auto_ptr<Translator> translator(new BytecodeTranslatorImpl);
  Code * code = NULL;
  std::vector<Var *> params;
  
  if (!ensureOkStatus(translator->translate(programText, &code))) {
    goto cleanup;
  }
  
  code->disassemble(); 
  ensureOkStatus(code->execute(params));

  cleanup:
  delete code;
}

#define TRACE_MAX 20

void printStackTrace(int sig) {
  void * traceEntries[TRACE_MAX];
  size_t traceEntriesCount = backtrace(traceEntries, TRACE_MAX);

  std::cout << "Error: signal " << sig << ":" << std::endl;
  backtrace_symbols_fd(traceEntries, traceEntriesCount, 2);
  std::exit(1);
}

 
int main(int argc, char** argv) {
 signal(SIGABRT, printStackTrace);
 if (argc != 2) {
    std::cout << "usage: " << argv[0] << " <filename>" << std::endl;
    return 0;
  }

  std::ifstream stream(argv[1]);
  if (!stream.is_open()) {
    std::cerr << "failed to open file: " << argv[1] << std::endl;
    return 1;
  }

  std::string programText((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
 
  runTranslator(programText);

  return 0;
}
