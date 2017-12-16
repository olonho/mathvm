#ifndef STUDENTS_2017_MARKELOV_TRANSLATOR_H_
#define STUDENTS_2017_MARKELOV_TRANSLATOR_H_

#include "parser.h"
#include "mathvm.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <stack>
#include <tuple>
#include <dlfcn.h>
#include <vector>
#include "code.h"

//#define TRANS_DEBUG

#ifdef TRANS_DEBUG
#define D(arg) std::cout << arg
#else
#define D(arg)
#endif

namespace mathvm {

class PublicTranslator: public Translator {
    void * priv;
public:
    ~PublicTranslator();
    PublicTranslator();
    Status* translate(const string& program, Code* *code) override;
};

}

#endif
