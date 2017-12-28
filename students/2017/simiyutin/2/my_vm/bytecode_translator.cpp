#include <fstream>
#include "../my_include/bytecode_translator_visitor.h"
#include "../my_include/interpreter.h"

using namespace mathvm;
using namespace std;
//#define DEBUG

Status *BytecodeTranslatorImpl::translate(const string &program, Code **code) {

    Parser parser;
    Status * status = parser.parseProgram(program);
    if (status->isError()) {
        return status;
    }

    BytecodeTranslatorVisitor visitor;
    FunctionNode * node = parser.top()->node();

    node->visitChildren(&visitor);

    Bytecode bytecode = visitor.getBytecode();
    map<string, int> topMostVars = visitor.getTopMostVars();
    vector<string> stringConstants = visitor.getStringConstants();
    std::map<uint16_t, uint32_t> functionOffsets = visitor.getFunctionOffsetsMap();
    std::map<uint16_t, std::pair<std::string, std::vector<mathvm::VarType>>> nativeFunctions = visitor.getNativeFunctions();
    (*code) = new Interpreter(bytecode, topMostVars, stringConstants, functionOffsets, nativeFunctions);
#ifdef DEBUG
    std::ofstream ofs("lastBytecode.txt");
    (*code)->disassemble(ofs);
#endif
    return status;
}
