#include <iostream>
#include <fstream>
#include "formatter.h"

using namespace std;
using namespace mathvm;

int main(int argc, char **argv) {
    if (argc < 2) {
      cout << "Expected mvm file as argument" << endl;
      return 0;
    }

    ifstream ifs(argv[1]);
    if (ifs.fail()) {
        cout << "Source file " << argv[1] << " does not exist" << endl;
    }

    string codeString((std::istreambuf_iterator<char>(ifs)),
    (std::istreambuf_iterator<char>()));

    Formatter f;
    string result;
    f.formatCode(result, codeString);

    if (argc < 3) {
        cout << result << endl;
        return 0;
    }

    ofstream ofs(argv[2]);
    if (ofs.fail()) {
        cout << "Could not write file " << argv[1] << endl;
        return 0;
    }

    ofs << result;
    return 0;
}