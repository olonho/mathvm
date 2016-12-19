//
// Created by natalia on 15.10.16.
//

#pragma once

//#include "mathvm.h"
#include "../../../include/mathvm.h"

namespace mathvm {

    class PrintTranslatorImpl : public Translator {

    public:
        virtual ~PrintTranslatorImpl();
        virtual Status* translate(const string& program, Code* *code);
    };

} //mathvm namespace