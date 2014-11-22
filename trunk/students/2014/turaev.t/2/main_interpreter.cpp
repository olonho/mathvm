#include <iostream>
#include <sstream>
#include "mathvm.h"

#include "parser.h"
#include "BytecodeVisitor.hpp"
#include "SimpleInterpreter.hpp"

using namespace mathvm;
using namespace std;

Status *BytecodeTranslatorImpl::translateBytecode(string const &source, InterpreterCodeImpl **code) {
    Parser parser;
    Status *status = parser.parseProgram(source);
    if (status && status->isError()) {
        return status;
    }

//        InterpreterCodeImpl *interpreterCode = new InterpreterCodeImpl();
    InterpreterCodeImpl *interpreterCode = new SimpleInterpreter();
    (*code) = interpreterCode;
    Context topContext(interpreterCode);
    topContext.introduceFunction(new BytecodeFunction(parser.top()));

    try {
        BytecodeVisitor visitor(&topContext);
        visitor.visitFunctionNode(parser.top()->node());
        interpreterCode->getBytecode()->addInsn(BC_STOP);
    } catch (TranslationError e) {
        return Status::Error(e.what(), e.where());
    }

    return Status::Ok();
}

Status *BytecodeTranslatorImpl::translate(string const &source, Code **code) {
    return translateBytecode(source, (InterpreterCodeImpl **) code);
}

bool printErrorIfNeeded(string module, char const *source, Status const *translateStatus) {
    if (translateStatus->isError()) {
        uint32_t position = translateStatus->getPosition();
        uint32_t line = 0, offset = 0;
        positionToLineOffset(source, position, line, offset);
        printf("Error in %s (expression at %d:%d): error '%s'\n",
                module.c_str(), line, offset, translateStatus->getError().c_str());
        return true;
    }
    return false;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <source_file_name.mvm>" << endl;
        return 1;
    }

    const char *source = loadFile(argv[1]);
    if (source == NULL) {
        cerr << "Cannot read file " << argv[1] << endl;
        return 1;
    }

    Translator *translator = new BytecodeTranslatorImpl();
    Code *code = NULL;
    Status *translateStatus = translator->translate(source, &code);
    if (printErrorIfNeeded("translator to bytecode", source, translateStatus)) {
        exit(100);
    }

    if (code) {
        LOG("-----------------------------");
#ifdef DEBUG
        code->disassemble(cout);
#else
        std::stringstream ss;
        code->disassemble(ss);
#endif
        LOG("------------RUN:-----------------");
        std::vector<Var *> vars;
        Status *interpreterStatus = code->execute(vars);
        if (printErrorIfNeeded("simple interpretator", source, interpreterStatus)) {
            exit(200);
        }
    } else {
        cout << "Code is null!" << endl;
    }

    delete translateStatus;
    delete translator;
    delete code;

    return 0;
}