#include <mathvm.h>
#include <parser.h>
#include "interpreter_code_impl.h"
#include "bytecode_visitor.h"


namespace mathvm {


    Status *BytecodeTranslatorImpl::translate(const string &program, Code **code) {
        InterpreterCodeImpl *interpreterCode = new InterpreterCodeImpl();
        *code =interpreterCode;
        Status *result = translateBytecode(program, &interpreterCode);
        return result;
    }

    Status *BytecodeTranslatorImpl::translateBytecode(const string &program, InterpreterCodeImpl **code) {
        Parser parser;
        Status *status = parser.parseProgram(program);
        if (status != 0) {
            if (status->isError()) return status;
        }
        ByteCodeVisitor *byteCodeVisitor = new ByteCodeVisitor();
        try{
        byteCodeVisitor->process(parser.top(), code);
        }catch (ErrorInfoHolder* error){
            return Status::Error(error->getMessage(), error->getPosition());
        }
        BytecodeFunction *mainFunction = dynamic_cast<BytecodeFunction *>((*code)->functionById(0));
        mainFunction->bytecode()->addInsn(BC_STOP);
        return Status::Ok();
    }


}