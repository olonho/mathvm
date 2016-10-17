#include "PrintVisitor.h"

#include <mathvm.h>
#include <parser.h>

namespace mathvm {

class PrintTranslatorImpl : public Translator {
public:
    Status* translate(const string& program, Code* *code) override {
        Parser parser;
        Status* status = parser.parseProgram(program);
        if (status->isError()) {
            return status;
        }

        PrintVisitor visitor(std::cout);
        parser.top()->node()->body()->visit(&visitor);

        return Status::Ok();
    }
};


Translator* Translator::create(const string& impl) {
    if (impl.empty() || impl == "printer") {
        return new PrintTranslatorImpl();
    }
    return nullptr;
}

}
