//
// Created by Владислав Калинин on 16.10.2018.
//

#ifndef MATHVM_SOURCE_TRANSLATOR_H
#define MATHVM_SOURCE_TRANSLATOR_H


#include "../../../include/mathvm.h"
#include "printer/print_visitor.h"
#include "../../../vm/parser.h"

namespace mathvm {
    class SourceTranslatorImpl : public Translator {
    public:
        Status *translate(const string &program, Code **code) override {
            Parser p;
            Status *status = p.parseProgram(program);
            if (status->isOk()) {
                AstFunction *astFunction = p.top();
                auto *print_visitor = new Print_visitor();
                astFunction->node()->visit(print_visitor);
                delete print_visitor;
            }
            return status;
        }
    };
}


#endif //MATHVM_SOURCE_TRANSLATOR_H
