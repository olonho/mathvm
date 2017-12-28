#pragma once

#include "includes.h"

namespace mathvm {
    struct AstPrinter: Translator {

        AstPrinter();

        Status *translate(const string &program, Code **code) override;

        virtual ~AstPrinter() override;
    };
}
