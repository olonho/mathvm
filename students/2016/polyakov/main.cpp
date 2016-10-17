#include <iostream>
#include <fstream>
#include "ast_printer.h"

using namespace std;
using namespace mathvm;

int main(int argc, char **argv) {
    if (argc < 3) {
        cout << "Not enough arguments - provide paths to file with mvm code and output file" << endl;
        return 0;
    }

    ifstream inputfs(argv[1]);
    if (inputfs.fail()) {
        cout << "Can't read file " << argv[1] << endl;
        exit(1);
    }

    string code((std::istreambuf_iterator<char>(inputfs)),
    (std::istreambuf_iterator<char>()));

    ASTPrinter printer;
    string result;
    Status* status = printer.print_code(result, code);

    cout << (status->isOk() ? "Success!" : status->getMsg()) << endl;

    ofstream outputfs(argv[2]);
    if (outputfs.fail()) {
        cout << "Can't not write file " << argv[1] << endl;
        cout << result << endl;
        exit(1);
    }

    outputfs << result;
    return 0;
}
