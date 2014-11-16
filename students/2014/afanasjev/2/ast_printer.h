#pragma once
#include "mathvm.h"

namespace mathvm {

class AstPrinter : public Translator {
public :
    Status* translate(const string& program, Code* *code);
};

}
