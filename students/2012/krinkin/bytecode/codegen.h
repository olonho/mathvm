#ifndef __CODE_GENERATOR_H__
#define __CODE_GENERATOR_H__

#include "mathvm.h"
#include "typechecker.h"
#include "asttranslator.h"

class CodeGenerator : public Translator
{
public:
	CodeGenerator() : Translator() {}
	virtual ~CodeGenerator() {}
	
	virtual Status* translate(const string& program, Code* *code);
};

#endif /* __CODE_GENERATOR_H__ */
