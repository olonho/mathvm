#include "translator_impl.h"

using namespace mathvm;

Status* TranslatorImpl::translate(const string& program, Code** code) 
{
    Parser parser;
    Status* status = parser.parseProgram(program);
    if (status->isError())
    {
        return status;
    }

    AstPrinter printer;
    parser.top()->node()->body()->visit(&printer);

    return Status::Ok();
}

Translator* Translator::create(const string& impl) 
{
    if (impl == "printer") 
    {
        return new TranslatorImpl();
    }

    return NULL;
}