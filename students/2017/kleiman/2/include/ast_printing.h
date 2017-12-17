#pragma once

#include "mathvm.h"

namespace mathvm {
    class ProgramTranslatorImpl : public Translator
    {
    public:
        virtual Status* translate(const string &program, Code **code);
        virtual ~ProgramTranslatorImpl() {}
    };
}