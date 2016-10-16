//
// Created by dsavvinov on 16.10.16.
//

#ifndef MATHVM_PRINTERTRANSLATOR_H
#define MATHVM_PRINTERTRANSLATOR_H

#include "../../../include/mathvm.h"

namespace mathvm {
class PrinterTranslator : public Translator {

public:
    virtual ~PrinterTranslator();

    virtual Status* translate(const string& program, Code** code) override;
};

}   // mathvm namespace
#endif //MATHVM_PRINTERTRANSLATOR_H
