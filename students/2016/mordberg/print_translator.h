#pragma once

#include <ostream>

#include "ast.h"
#include "mathvm.h"

namespace mathvm 
{

class PrintTranslator : public Translator {
public:
    PrintTranslator(std::ostream& out): _out(out) {}

    Status* translate(const std::string& program, Code* *code);

private:
    std::ostream& _out;
    
};

}
