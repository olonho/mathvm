#include <iostream>
#include <string>

#include "transformer.h"
#include "../../../vm/parser.h"
#include "ast_to_bytecode.h"
#include "bytecode_interpreter.h"

using namespace std;
using namespace mathvm;

class empty_code : public Code
{
    Status* execute(vector<Var *> &) { return Status::Ok(); }
};

void translate(const string& code_str)
{
    BytecodeTranslatorImpl translator;

    Code* code = new empty_code();
    Status *translate_status = translator.translate(code_str, &code);

    if (translate_status->isError())
        cout << "translation error: " << translate_status->getMsg() << endl;
    else
        code->disassemble();

    delete translate_status;
    delete code;
}

void interpreter(const string& code_str)
{
    BytecodeTranslatorImpl translator;

    Code* code = new bytecode_interpreter();
    Status *translate_status = translator.translate(code_str, &code);

    if (translate_status->isError())
        cout << "translation error: " << translate_status->getMsg() << endl;
    else
    {
        vector<Var*> empty;
        code->execute(empty);
    }

    delete translate_status;
    delete code;
}

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

    translate(code);
//    interpreter(code);

//    transform(code);

    return 0;
}
