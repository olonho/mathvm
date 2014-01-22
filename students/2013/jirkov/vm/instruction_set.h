#ifndef INSTRUCTION_SET
#define INSTRUCTION_SET
#include "../../../../include/mathvm.h"
namespace mathvm {
#include <setjmp.h>

#define MNEMONIC(b,s,l) #b,

static const char* const bytecode_mnemonic[] = {
FOR_BYTECODES( MNEMONIC )
	"TERMINATOR"
	};
#undef MNEMONIC
extern jmp_buf stop_label;
}
#endif
