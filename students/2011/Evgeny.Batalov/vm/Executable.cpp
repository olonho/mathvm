#include <iostream>
#include "Executable.h"

Executable::Executable()
  : stringConstantCounter(0) {
    using namespace mathvm;
}

mathvm::Status* Executable::execute(std::vector<mathvm::Var*, std::allocator<mathvm::Var*> >& vars) {
    //FIXME: implement me!
    return 0;
}

void Executable::disassemble(std::ostream& out, mathvm::FunctionFilter *f) const {
  using namespace mathvm;
  IdToFunction::const_iterator fit = idToFunction.begin();
  for(; fit != idToFunction.end(); ++fit) {
    TranslatedFunction *ft = fit->second;
    MyBytecodeFunction *fb = dynamic_cast<MyBytecodeFunction*>(ft);
    std::cout << std::endl  << fit->first << ":" << fb->name() << ":"  << std::endl;
    fb->disassemble(out);
  }
}


