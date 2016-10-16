//
// Created by dsavvinov on 16.10.16.
//

#include "PrinterTranslator.h"
#include "../../../vm/parser.h"
#include "PrinterVisitor.h"

namespace mathvm {

Status* PrinterTranslator::translate(const string &program, Code **code) {
    Parser parser;

    Status * parseStatus = parser.parseProgram(program);
    if (parseStatus->isError()) {
        return parseStatus;
    }

    PrinterVisitor visitor;
    // skip visiting topmost fake function
    parser.top()->node()->asFunctionNode()->body()->visit(&visitor);

    return Status::Ok();
}

PrinterTranslator::~PrinterTranslator() { }

} // "mathvm" namespace