#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

#include "parser.h"
#include "codeimpl.hpp"
#include "translatorvisitor.hpp"
#include "translationerror.hpp"
#include "context.hpp"
#include <signal.h>
#include <cstdlib>
#include <cstring>

using namespace mathvm;

Status* BytecodeTranslatorImpl::translate(std::string const& program, Code** code) {
    Status* status  = 0;
    Parser parser;
    status = parser.parseProgram(program);
    if (status)
        return status;

    CodeImpl* codei = new CodeImpl();
    (*code) = codei;
    Context root(codei);
    root.addFunction(new BytecodeFunction(parser.top()));
    try {
        TranslatorVisitor(&root).visitFunctionNode(parser.top()->node());
//        TranslatorVisitor(&root).run(parser.top());
        codei->bytecode()->addInsn(BC_STOP);
    } catch (TranslationError e) {
        return new Status(e.what(), e.pos());
    }
    return NULL;
}

void segfault_handler(int signal, siginfo_t* si, void* arg) {
    exit(0);
}

int main(int argc, char* argv[]) {

    struct sigaction sa;

    memset(&sa, 0, sizeof(struct sigaction));
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = segfault_handler;
    sa.sa_flags   = SA_SIGINFO;

    sigaction(SIGSEGV, &sa, NULL);

    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <mvm source>" << std::endl;
        return 0;
    }

    std::fstream input(argv[1]);
    if (!input.is_open()) {
        std::cerr << "Couldn't open file: " << argv[1] << std::endl;
        return -1;
    }

    std::string source((std::istreambuf_iterator<char>(input)),
                        std::istreambuf_iterator<char>());

    Translator* t = new BytecodeTranslatorImpl();
    Code* c = 0;
    Status* s = t->translate(source, &c);
    if (s) { // error
        std::cerr << "Error at " << s->getPosition() << ": " << s->getError() << std::endl;
        delete s;
        return -1;
    }

    if (c) {
        LOGGER << "======================================" << std::endl;
        c->disassemble(std::cout);
        LOGGER << "======================================" << std::endl;
        std::vector<Var*> vars;
//        Status* s = c->execute(vars);
//        if (s) {
//            std::cerr << "Error at " << s->getPosition() << ": " << s->getError() << std::endl;
//        }
    } else {
        delete t;
        std::cerr << "code is NULL" << std::endl;
        return -1;
    }
    delete t;
    delete c; 
    return 0;
}
