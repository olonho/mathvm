#include <fstream>
#include <iostream>
#include <memory>
#include <streambuf>
#include <string>
#include "../../../include/mathvm.h"

using namespace mathvm;
using namespace std;
using namespace std::literals::string_literals;

string loadFile(const string &filename) {
    ifstream file{filename};
    if (!file) {
        cerr << "Cannot read file: " << filename << endl;
        exit(1);
    }
    return {(istreambuf_iterator<char>(file)), istreambuf_iterator<char>()};
}

int main(int argc, char **argv) {
    string impl;
    string script_file;

//    Bytecode b1;
//    Label l1(&b1);
//    Label l2(&b1);
//
////    b1.addBranch(BC_IFICMPE, l1);
//    b1.addInsn(BC_CALL);
//    b1.addInt16(10);
////    b1.addBranch(BC_JA, l2);
////    b1.bind(l1);
////    b1.addInsn(BC_DMUL);
////    b1.bind(l2);
//    b1.addInsn(BC_DLOAD0);
//    b1.dump(cout);
//    return 0;

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

    Code *code = 0;
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

//    if (impl != "printer") {
//        assert(code);
//
//        vector<Var *> vars;
//        unique_ptr<Status> execStatus{code->execute(vars)};
//
//        if (execStatus->isError()) {
//            cerr << "Cannot execute expression, error: " << execStatus->getErrorCstr();
//        }
//
//        delete code;
//    }

    return 0;
}
