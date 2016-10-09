#include <iostream>
#include <fstream>
#include "formatter.h"

int main(int argc, char **argv) {
    using namespace std;
    using namespace mathvm;

    if (argc < 2) {
        cout << "Expected mvm file as argument" << endl;
        return -1;
    }

    ifstream ifs(argv[1]);
    if (ifs.fail()) {
        cout << "Failure opening " << argv[1] << endl;
        return -1;
    }

    string codeIn = string(std::istreambuf_iterator<char>(ifs),
                           std::istreambuf_iterator<char>());
    string codeOut;
    Formatter::formatCode(codeIn, codeOut);

    if (argc < 3) {
        cout << codeOut << endl;
        return 0;
    }

    ofstream ofs(argv[2]);
    if (ofs.fail()) {
        cout << "Could not write to file " << argv[1] << endl;
        return -1;
    }

    ofs << codeOut;
    return 0;
}
