#include "mathvm.h"
#include "BCTranslatorImpl.h"

namespace mathvm {

  Translator* Translator::create(const string& impl) {
    if (impl == "" || impl == "intepreter") {
      return new BCTranslatorImpl();
    }
    if (impl == "jit") {
      throw std::runtime_error("Not implemeted for array extension");
    }
    assert(false);
    return 0;
  }

}

