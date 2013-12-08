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
    cerr << "File: " << filename << "  does not exist. " 
         << endl;
}

void translationError(Status* s) {
    cout << "There is some error while translating. Error message:\n"
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
        translationError(s);

        release(code, translator);
        return 1;
    }

    code->disassemble(cout);

    release(code, translator);

    return 0;
}