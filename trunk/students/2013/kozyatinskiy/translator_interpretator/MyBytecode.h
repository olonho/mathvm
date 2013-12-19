#pragma once

class MyInstruction;

#include <ostream>
using std::ostream;
#include <vector>
using std::vector;

class MyBytecode
{
public:
	MyBytecode(void);
	~MyBytecode(void);

	void addInstruction(MyInstruction* instruction);
	void dump(ostream& os);

	void removeLabels();

	MyInstruction* take(int num);

	MyInstruction* get(int num) const;

	size_t instructionCount() const;

private:
	MyBytecode(const MyBytecode& bytecode);
	vector<MyInstruction*> instructions_;
};

