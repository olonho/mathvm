#include "MyInterpreter.h"

#include "MyInstruction.h"
#include "MyBytecode.h"

MyInterpreter::MyInterpreter(MyBytecode* bytecode) : bytecode_(bytecode){}

MyInterpreter::~MyInterpreter(void)
{
	delete bytecode_;
}

void MyInterpreter::run()
{
	if (bytecode_->instructionCount() == 0) return;
	state.isExit_ = false;
	while (!state.isExit_)
	{
		MyInstruction* i = bytecode_->get(state.programPointer_);
		//cout << i->text() << endl;
		i->exec(&state, &memoryManager);
	}
}
