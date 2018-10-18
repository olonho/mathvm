#include <mathvm.h>
#include <parser.h>
#include <visitors.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <memory>
#include <streambuf>
#include <string>

using namespace mathvm;
using namespace std;
using namespace std::literals::string_literals;

string loadFile(const string& filename) {
    ifstream file{filename};
    if (!file) {
        cerr << "Cannot read file: " << filename << endl;
        exit(1);
    }
    return {istreambuf_iterator<char>(file), istreambuf_iterator<char>()};
}

int main(int argc, char** argv) {
    string impl;
    string script_file;

    if (argc == 3) {
        if (argv[1] == "-p"s) {
            impl = "printer";
        } else if (argv[1] == "-i"s) {
            impl = "interpreter";
        } else if (argv[1] == "-j"s) {
            impl = "jit";
        } else {
            cerr << "Invalid option: " << argv[1] << endl;
            return 1;
        }
        script_file = argv[2];
    } else if (argc == 2) {
        impl = "interpreter";
        script_file = argv[1];
    } else {
        cerr << "Usage: mvm [OPTION] FILE" << endl;
        return 1;
    }

    string program = loadFile(script_file);

    unique_ptr<Translator> translator{Translator::create(impl)};

    if (!translator) {
        cout << "TODO: Implement translator factory in translator.cpp" << endl;
        return 1;
    }

    Code* code = nullptr;
    unique_ptr<Status> translateStatus{translator->translate(program, &code)};

    if (translateStatus->isError()) {
        uint32_t position = translateStatus->getPosition();
        uint32_t line = 0, offset = 0;
        positionToLineOffset(program, position, line, offset);
        cout << "Cannot translate expression: "
             << "expression at " << line << "," << offset << "; "
             << "error '" << translateStatus->getErrorCstr() << "'" << endl;
        return 1;
    }

    if (impl != "printer") {
        assert(code);

        vector<Var*> vars;
        unique_ptr<Status> execStatus{code->execute(vars)};

        if (execStatus->isError()) {
            cerr << "Cannot execute expression, error: " << execStatus->getErrorCstr();
        }

        delete code;
    }

    return 0;
}
