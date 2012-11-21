#ifndef __BC_TRANSLATOR_H__
#define __BC_TRANSLATOR_H__

#include "mathvm.h"
#include "parser.h"

#include "generator.h"
#include "bccode.h"
#include "typer.h"

using namespace mathvm;

class BCTransaltor : public Translator
{
public:
	BCTransaltor() : Translator() {}
	virtual ~BCTransaltor() {}
	
	virtual Status* translate(std::string const &program, Code **code)
	{
        Parser parser;
        Status *parse_status(parser.parseProgram(program));
	    if (parse_status) return parse_status;
	
	    Typer typer;
	    Status *typer_status(typer.check(parser.top()));
	    if (typer_status) return typer_status;

        BCCode *bcode = new BCCode();
        Generator generator;
        generator.translate(parser.top(), typer.types(), bcode);
        *code = bcode;
        
        delete typer.types();

        return 0;
	}
};

#endif /* __BC_TRANSLATOR_H__ */
