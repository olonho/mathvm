#include "mathvm.h"
#include "ast_pretty_printer.h"
#include "parser.h"


namespace mathvm
{

struct AstPrettyPrinterTranslator : Translator 
{
    
    Status * translate(const string & program, Code ** code)  
    {
        Parser parser{};    
        Status * status = parser.parseProgram(program);
    
        if (status->isOk()) 
        {
            AstPrettyPrinterVisitor printer{};
            parser.top()->node()->visitChildren(&printer);
        }
    
        return status;
    } 
};

Translator * Translator::create(const string & impl) 
{
    if (impl == "printer") 
    {
        return new AstPrettyPrinterTranslator{};
    }
    else 
    {
        return nullptr;
    }
};

}
