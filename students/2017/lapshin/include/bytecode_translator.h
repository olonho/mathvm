#pragma once

#include "visitors.h"

namespace mathvm::ldvsoft {

class BytecodeTranslator : public Translator {
private:
	class TranslationData;
	class Visitor;

public:
	BytecodeTranslator() = default;
	virtual ~BytecodeTranslator() override = default;
	virtual Status *translate(string const &program, Code **code_ptr) override;
};

}
