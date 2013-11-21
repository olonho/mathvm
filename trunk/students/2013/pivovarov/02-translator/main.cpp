#include "mathvm.h"

#include "InterpreterCodeImpl.h"

#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <iostream>

using namespace mathvm;
using namespace std;

int main(int argc, char** argv) {
	if(argc != 2) {
      cout << "Usage: " << argv[0] << " <input>" << endl;
      return 1;
	}

	char const * program = loadFile(argv[1]);
	if (program == NULL) {
		cout << "Cannot read file" << endl;
		return 1;
	}

	Code * code;
	BytecodeTranslatorImpl translator;

	Status * status = translator.translate(program, &code);
	if (status->isError()) {
		cout << status->getError() << endl;
	} else {
		code->disassemble();
		delete status;

		status = ((InterpreterCodeImpl*)code)->execute();
		if (status->isError()) {
			cout << status->getError() << endl;
		}

		delete code;
	}

	delete status;
	delete [] program;

	return 0;
}
