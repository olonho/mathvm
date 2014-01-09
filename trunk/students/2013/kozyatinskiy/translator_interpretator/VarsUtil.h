#ifndef _VARS_UTIL_H
#define _VARS_UTIL_H

#include <mathvm.h>
using namespace mathvm;

void load0(Bytecode* bytecode, VarType t);
void convert(Bytecode* bytecode, VarType from, VarType to);
void loadVar(Bytecode* bytecode, VarType t, uint32_t offset);
void print(Bytecode* bytecode, VarType t);
void popToVar0(Bytecode* bytecode, VarType t);
void pushFromVar0(Bytecode* bytecode, VarType t);
void storeVar(Bytecode* bytecode, VarType t, uint32_t offset);

int sizeOfType(VarType t);

typedef void* PointerType;
const int PointerSize = sizeof(PointerType);

#endif
