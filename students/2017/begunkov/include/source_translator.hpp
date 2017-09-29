#include "mathvm.h"
#include "visitors.h"

namespace mathvm {

    class SourceTranslatorImpl
        : public Translator
    {
        virtual ~SourceTranslatorImpl() {}

        virtual Status *translate(const string &program, Code **code);
    };

}; // ::mathvm
