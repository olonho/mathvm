#pragma once
#include <vector>


struct StackVariable {
	union {
		int64_t int_part;
		double double_part;
		const char * string_part;
	};
};
 
struct Frame {
	Frame * parentFrame;
	Frame * previousFrame;
	std :: map <int16_t, StackVariable> variables;
	int16_t function_id;
	int32_t stackPointer;
};

class GeneratedCode : public mathvm :: Code {
	mathvm :: Bytecode* currentBytecode;
	Frame * currentFrame;
	
	int dataPointer;
	std :: vector<StackVariable> dataStack;
	public:
		virtual mathvm::Status* execute(std::vector<mathvm::Var*> & vars);	
		mathvm :: Instruction getNextInstruction();
		void add(int64_t item);
		void add(double item);
		void add(const char* item);
		void add(StackVariable item);
		void addString(int id);
		int64_t nextInt();
		int16_t nextInt16();
		double nextDouble();
		
		const char* getString();
		int64_t getInt();
		double getDouble();
		StackVariable getVar();
		void print(const char* str);
		void print(int64_t item);
		void print(double item);
		void jump(int16_t to);
		void setBytecode(mathvm :: Bytecode* bytecode) {
			currentBytecode = bytecode;
		}
		Frame* loadFrame(int16_t fid) ;

		void createFrame(int16_t fid) ;
		void dropFrame() ;
};