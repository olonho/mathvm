#ifndef MATHVM_TRANSLATOR_IMPL_H
#define MATHVM_TRANSLATOR_IMPL_H

#include "mathvm.h"

namespace mathvm {

class TranslatorForPrinterImpl : public Translator {
public:
    TranslatorForPrinterImpl() = default;
    
    Status *translate(const string &program, Code **code) override;
};


class MyBytecodeTranslator : public Translator {
    bool print_;

public:
    explicit MyBytecodeTranslator(bool print=true)
        : print_(print)
    {}
    
    Status *translate(const string &program, Code **code) override;
};

}

#endif //MATHVM_TRANSLATOR_IMPL_H
