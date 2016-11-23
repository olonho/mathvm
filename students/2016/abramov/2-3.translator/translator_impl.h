#pragma once

#include <parser.h>
#include <mathvm.h>

namespace mathvm
{
    class TranslatorImpl : public Translator
    {
        public:
            Status* translate(const string& program, Code** code) override;
    };
}