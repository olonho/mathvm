#ifndef AST_PRINTER_H
#define AST_PRINTER_H

#include <string>
#include "mathvm.h"

using std::string;
using mathvm::Translator;
using mathvm::Status;
using mathvm::Code;

class AstPrinter : public Translator {

public:
    virtual Status *translate(const string & program, Code **);
};


#endif // AST_PRINTER_H
