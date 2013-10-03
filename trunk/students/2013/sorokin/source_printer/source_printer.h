#ifndef SOURCE_PRINTER_H
#define SOURCE_PRINTER_H


#include "visitors.h"
#include "string"
#include "iostream"


using std::string;
using namespace mathvm;

class source_printer : public AstBaseVisitor {
private: 
    std::ostream &my_ostream;

private:
    int my_indent;
 
    string get_indent_line();

public:
    source_printer( std::ostream & out_stream = std::cout );

    #define VISITOR_FUNCTION(type, name)            \
    virtual void visit##type(type* node);

    FOR_NODES(VISITOR_FUNCTION)
    #undef VISITOR_FUNCTION

    virtual ~source_printer();
};

#endif
