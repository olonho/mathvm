#include <iostream>

#include "mathvm.h"
#include "parser.h"
#include "pretty_print.h"
#include "code_maker.h"
#include "code_interpreter.h"

namespace mathvm {

    class PPTranslator : public Translator {

    public:
        virtual ~PPTranslator() override {}

        virtual Status *translate(const string &program, Code **code) override {
            Parser parserInstance = Parser();
            Status *parseStatus = parserInstance.parseProgram(program);
            if (parseStatus->isError()) {
                return parseStatus;
            }
            delete parseStatus;

            pretty_print *visitor = new pretty_print();
            parserInstance.top()->node()->visitChildren(visitor);
            delete visitor;

            return Status::Ok();
        }
    };

    class CMTranslator : public Translator {

    public:
        virtual ~CMTranslator() override {}

        virtual Status *translate(const string &program, Code **code) override {
            Parser parserInstance = Parser();
            Status *parseStatus = parserInstance.parseProgram(program);
            if (parseStatus->isError()) {
                return parseStatus;
            }
            delete parseStatus;


            code_interpreter* code1 = new code_interpreter();
            try {
                auto tFunction = new BytecodeFunction(parserInstance.top());
                code1->addFunction(tFunction);
                code_maker *visitor = new code_maker(code1, tFunction->bytecode());
                visitor->add_scope();
                parserInstance.top()->node()->visit(visitor);
                tFunction->setLocalsNumber(visitor->locals_number());
                visitor->pop_scope();
                delete visitor;
            } catch (const char* msg) {
                return Status::Error(msg);
            }

            *code = code1;

            return Status::Ok();
        }
    };

    Translator *Translator::create(const string &impl) {
        if (impl == "printer") {
            return new PPTranslator();
        }

        return new CMTranslator();
    }

}
