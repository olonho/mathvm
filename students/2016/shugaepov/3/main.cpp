#include <iostream>
#include <string>

#include "../../../../vm/parser.h"
#include "ast_to_bytecode.h"
#include "bytecode_interpreter.h"

using namespace std;
using namespace mathvm;

void interpreter(const string& code_str)
{
    BytecodeTranslatorImpl *translator = new BytecodeTranslatorImpl();

    Code* code = new bytecode_interpreter();
    Status *translate_status = translator->translate(code_str, &code);

    if (translate_status->isError())
        cout << "translation error: " << translate_status->getMsg() << endl;
    else
    {
        vector<Var*> empty;
        code->execute(empty);
    }

    delete translate_status;
    delete code;
    delete translator;
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

    interpreter(code);

    return 0;
}
