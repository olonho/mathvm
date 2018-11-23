#ifndef source_translator_hpp_INCLUDED
#define source_translator_hpp_INCLUDED
#include <mathvm.h>

namespace mathvm {

class SourceTranslatorImpl : public Translator {
  public:
    Status* translate(const string& program, Code* *code);
};

}

#endif // source_translator_hpp_INCLUDED

