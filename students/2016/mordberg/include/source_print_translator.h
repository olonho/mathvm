#pragma once

#include <ostream>

#include "ast.h"
#include "mathvm.h"

namespace mathvm 
{

class SourcePrintTranslator : public Translator {
public:
    SourcePrintTranslator(std::ostream& out): _out(out) {}

    Status* translate(const std::string& program, Code* *code);

private:
    std::ostream& _out;
    
};

}
