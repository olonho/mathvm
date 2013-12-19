#include "CodeImpl.h"

#include <iostream>
#include <sstream>
#include <fstream>

using namespace mathvm;

// utils

string getSource(istream& in) {
    stringstream stream; 
    stream << in.rdbuf();
    return stream.str();
}

void release(Code* code, Translator* translator) {
    delete code;
    delete translator;
}

void usage() {
    cerr << "USAGE: <source filename>. " << endl;
}

void fileNotFound(const string& filename) {
    cerr << "File: " << filename << " does not exist. " 
         << endl;
}

void translationError(const char* source, Status* s) {
    uint32_t position = s->getPosition();
    uint32_t line = 0, offset = 0;
    positionToLineOffset(source, position, line, offset);
    printf("Cannot translate expression: expression at %d,%d; "
           "error '%s'\n",
           line, offset,
           s->getError().c_str());

    delete s;
}

void runtimeError(Status* s) {
    cerr << "There is some error while executing. Error message: "
         << s->getError() << endl;

    delete s;
}

int main(int argc, char const *argv[]) {

    if (argc != 2) {
        usage();
        return 1;
    }

    string filename = argv[1];

    ifstream input(filename.c_str());
    if (!input) {
        fileNotFound(filename);
        return 1;
    }

    string source(getSource(input));

    Translator* translator = new BytecodeTranslatorImpl;
    Code* code = new CodeImpl;

    if (Status* s = translator->translate(source, &code)) {
        translationError(source.c_str(), s);

        release(code, translator);
        return 1;
    }

    // code->disassemble(cout);

    if (Status* s = ((CodeImpl*)code)->execute()) {
        runtimeError(s);

        release(code, translator);
        return 1;
    }

    release(code, translator);

    return 0;
}