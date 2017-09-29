#pragma once

#include "mathvm.h"

using namespace mathvm;

struct AstPrinterTranslatorImpl: Translator {

    AstPrinterTranslatorImpl();

    Status *translate(const string &program, Code **code);

    virtual ~AstPrinterTranslatorImpl() override;
};