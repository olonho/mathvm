#ifndef BYTECODE_TRANSLATOR_H
#define BYTECODE_TRANSLATOR_H

#include "interpreter/typer.h"
#include "interpreter/ast_to_bytecode.h"
#include "../../../include/mathvm.h"
#include "interpreter/interpreter.h"
#include <mathvm.h>


namespace mathvm {

    Status* BytecodeTranslatorImpl::translate(const string& program, Code** code) {
        Parser parser = Parser();
        Status* execution_status = Status::Ok();
        parser.parseProgram(program);
        //Status* parse_status = parser.parseProgram(program);
        const AstFunction* top = parser.top();
        
        
        FunctionNode* mainFunction = top->node();
        //AstToBytecodeVisitor* a = new AstToBytecodeVisitor(*code);
        Typer* typer = new Typer(top->scope(), mainFunction);
        try {
            mainFunction->visit(typer);
        }        catch (const runtime_error& e) {
            execution_status = Status::Error(e.what(), 0);
        }
        *code = new CodeImpl();
        //Bytecode* bytecode = new Bytecode();
        AstToBytecodeVisitor* to_bytecode = new AstToBytecodeVisitor(*code, top->scope());
        to_bytecode->visitMain(top);
        //bytecode->dump(cout);
        
        return execution_status;
    }
}

#endif /* BYTECODE_TRANSLATOR_H */

