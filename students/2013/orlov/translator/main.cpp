#include "TranslatorVisitor.h"

int main(int argc, char * argv[]) {
	if (argc != 2) {
		std::cout << "Usage program <src>" <<std::endl;
		return -1;
	}

	char * src = mathvm::loadFile(argv[1]);

	mathvm::TranslatorVisitor translator;
	mathvm::CodeImpl * code = 0;

	mathvm::Status * res = translator.translate(src, &code);
	if(res && res->isError()) {
		std::cout << "Translation error: "  << res->getError();
		return -1;
	}
}
