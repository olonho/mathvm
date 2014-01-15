#pragma once

//#define WIN

#ifndef WIN
#include <dlfcn.h>
#define ASSEMBLER Assembler
#define IFST rdi
#define ISND rsi
#define ITHD rdx
#define IFTH rcx
#define LONG_FORMAT "%ld"
#define VARTYPE
#else
#define ASSEMBLER X86Assembler
#define IFST rcx
#define ISND rdx
#define ITHD r8
#define IFTH r9
#define LONG_FORMAT "%I64d"
#define VARTYPE mathvm::VarType::
#endif
