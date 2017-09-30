#pragma once 

#include "visitors.h"

namespace mathvm 
{

struct AstPrettyPrinterVisitor : AstBaseVisitor 
{
    
#define VISITOR_FUNCTION(type, name) \
void visit##type(type* node) override;

FOR_NODES(VISITOR_FUNCTION)
#undef VISITOR_FUNCTION

private:
    static string escaped(const string & value);
      
    enum Special {
        SEMICOLON_ENDL,
        INC_INDENT,
        DEC_INDENT,
        SKIP_NEXT_SEMICOLON,
        INDENT
    };

    struct Printer {    
        Printer & operator<<(const string & value);
        Printer & operator<<(int64_t value);
        Printer & operator<<(double value);
        Printer & operator<<(Special value);
        Printer & operator<<(VarType value);
        Printer & operator<<(const AstVar * value);
        Printer & operator<<(TokenKind value);

    private:
        bool skipNextSemicolon = false;
        int indentSize = 0; 
    };

    Printer out{};
};


} // namespace mathvm