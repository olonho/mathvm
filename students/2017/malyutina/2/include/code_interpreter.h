#ifndef CODE_INTERPRETER_H__
#define CODE_INTERPRETER_H__

#include "../../../../../include/mathvm.h"
#include "execution_exception.h"

#include <vector>
#include "data.h"
#include "local_variable_storage.h"

using namespace mathvm;

class code_interpret {
public:
    typedef std::vector<data> stack;
    typedef std::pair<data, data> data_pair;
    typedef std::vector<int32_t> stack_counter;
    typedef std::vector<TranslatedFunction *> stack_function;

    const size_t alloc_memory = 1024 * 1024;

    code_interpret(Code *code);

    void execute();

private:
    void preallocateMemory();

    void executeInstruction();

    void call();

    void ja();

    void ificmpne();

    void ificmpe();

    void ificmpg();

    void ificmpge();

    void ificmpl();

    void ificmple();

    void loadivar();

    void loaddvar();

    void loadsvar();

    void storeivar();

    void storedvar();

    void storesvar();

    void loadctxivar();

    void loadctxdvar();

    void loadctxsvar();

    void storectxivar();

    void storectxdvar();

    void storectxsvar();

    void dload();

    void iload();

    void sload();

    void dload0();

    void iload0();

    void sload0();

    void iload1();

    void dload1();

    void iloadm1();

    void dloadm1();

    void iadd();

    void dadd();

    void isub();

    void dsub();

    void imul();

    void dmul();

    void idiv();

    void ddiv();

    void imod();

    void ineg();

    void dneg();

    void iaor();

    void iaand();

    void iaxor();

    void iprint();

    void dprint();

    void sprint();

    void i2d();

    void d2i();

    void bc_return();

    void swap();

    void icmp();

    void dcmp();

    void dump();

    int32_t programCounter();

    int32_t stackFrame();

    void push(data holder);

    void spush(int16_t value);

    void ipush(int64_t value);

    void dpush(double value);

    void pop();

    data top();

    data_pair topPair();

    data tos();

    Bytecode *currentBytecode();

    TranslatedFunction *currentFunction();

    void newContext(int32_t localsNumber);

    void popContext();

    void addRelativeOffset(int16_t relativeOffset);

    int16_t int16();

    void next(size_t i = 1);

    void next2();

    void next3();

    void next4();

    void next8();

    void newCounter();

    const std::string topFunctionName = "<top>";

    stack_counter stack_counter_prog;

    stack_counter _stack_frame;
    stack_function _function_stack;
    Code *_code;
    stack _data_stack;
    stack _stack_local_var;
    const int16_t _id_string;
    variable_storage _storage_local_variable;
};

#endif
