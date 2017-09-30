#pragma once

#include "mathvm.h"

namespace mathvm
{

class PrinterTranslator final : public Translator
{
public:
    ~PrinterTranslator() override
    {}
    Status* translate(string const& program, Code** code) override;
};

}
