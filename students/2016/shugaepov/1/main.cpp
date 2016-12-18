#include <iostream>
#include <string>

#include "transformer.h"
#include "../../../../vm/parser.h"

using namespace std;
using namespace mathvm;


void transform(const string& code)
{
    Parser parser;
    Status *s = parser.parseProgram(code);
    if (!s->isOk())
    {
        cout << "parse error\n";
        return;
    }

    transformer tr(true);

    parser.top()->node()->body()->visit(&tr);

    cout << tr.to_str() << endl;
    delete s;
}

int main(int argc, char** argv)
{
    string code = "";
    if (argc != 2)
    {
        cout << "specify input file name\n";
        return 1;
    }
    code = string(loadFile(argv[1]));

    transform(code);

    return 0;
}
