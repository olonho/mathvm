#include "printer.h"
#include "parser.h"
#include "bytecode_translator.h"

using namespace mathvm;
using namespace mathvm::ldvsoft;

class AstPrinterTranslator: public Translator {
private:
	AstPrinterStyle style;

public:
	AstPrinterTranslator(AstPrinterStyle const &style);
	virtual ~AstPrinterTranslator() = default;
	virtual Status* translate(string const &program, Code **code);
};

AstPrinterTranslator::AstPrinterTranslator(AstPrinterStyle const &style):
	style(style) {}

Status *AstPrinterTranslator::translate(string const &program, Code **code) {
	(void) code;
	Parser parser{};
	Status *status{parser.parseProgram(program)};
	if (!status->isOk())
		return status;
	delete status;
	AstPrinter printer(style);
	string printed{printer.print(parser.top()->node())};
	cout << printed;
	return Status::Ok();
}

Translator *Translator::create(string const &name) {
	if (name == "printer")
		return new AstPrinterTranslator(AstPrinter::testStyle());
	if (name == "printer-pretty")
		return new AstPrinterTranslator(AstPrinter::prettyStyle());
	if (name == "to-bytecode" || name == "execute-bytecode")
		return new BytecodeTranslator();
	return nullptr;
}
