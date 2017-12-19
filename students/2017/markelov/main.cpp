#include "parser.h"
#include "mathvm.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <stack>

#include "translator.h"
#include "printer.h"

using namespace mathvm;
using namespace std;

#if defined(INTERPRET_DEBUG) || defined(TRANS_DEBUG)
#define DDI(arg) std::cout << arg
#else
#define DDI(arg)
#endif

#define PRINTER



int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Wrong number of args";
        return 1;
    }
    std::string src(argv[1]);
    std::ifstream src_stream (src);
    std::string prog((std::istreambuf_iterator<char>(src_stream)),
                     std::istreambuf_iterator<char>());

#ifndef PRINTER
    Code* code = 0;
    vector<Var*> vars;
    Translator * tr = Translator::create();
    Status * translate_st = tr->translate(prog, &code);
    Status * exec_status = 0;
    if (translate_st->isError()) {
        std::cout << translate_st->getError() << std::endl;
        goto out;
    }

    exec_status = code->execute(vars);
    if (exec_status->isError()) {
        std::cout << exec_status->getError() << std::endl;
        goto out;
    }

    for (auto varr : vars) {
        Var * var = varr;
        cout << endl << var->name() << " ";
        switch(var->type()) {
        case VT_INT:
            cout << var->getIntValue();
            break;
        case VT_DOUBLE:
            cout << var->getDoubleValue();
            break;
        case VT_STRING:
            cout << var->getStringValue();
            break;
        default:
            break;
        }
        cout << endl;
        delete var;
    }
out:
    delete tr;
    if (code != nullptr)
        delete code;
    delete translate_st;
    if (exec_status)
        delete exec_status;
    return 0;
#else
    Parser parser;
    Status * parse_stat = parser.parseProgram(prog);
    if (parse_stat->isError()) {
        cout << parse_stat->getError();
    } else {
        PrettyPrinter printer(parser.top());
    }
    delete parse_stat;
#endif
}
