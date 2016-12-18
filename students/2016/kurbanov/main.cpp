#include <iostream>
#include <fstream>
#include <sstream>
#include <streambuf>
#include <memory>

#include "parser.h"
#include "include/printer.h"
#include "include/interpreter.h"

typedef std::shared_ptr<mathvm::Status> StatusPtr;
typedef std::shared_ptr<mathvm::Translator> TranslatorPtr;

using namespace mathvm;

int test_printer(int argc, char** argv) {
    std::string filename(argv[1]);
    std::ifstream file(filename);
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());

    mathvm::Parser parser;
    mathvm::Status* parsingStatus = parser.parseProgram(content);
    if (!parsingStatus->isOk()) {
        std::cerr << parsingStatus->getPosition() << " : " <<  parsingStatus->getError();
        return 1;
    }

    std::stringstream sstream;
    mathvm::AstPrinterVisitor printer(sstream);
    parser.top()->node()->visitChildren(&printer);

    if (argc == 3) {
        std::ofstream filestream(argv[2], std::ofstream::out);
        filestream << sstream.str() << std::endl;
        return 0;
    }
    std::cout << sstream.str() << std::endl;
    return 0;
}

int test_translator(int argc, char** argv) {
    std::string impl = "translator";
    const char* script = NULL;
    for (int32_t i = 1; i < argc; i++) {
        if (std::string(argv[i]) == "-j") {
            impl = "jit";
        } else {
            script = argv[i];
        }
    }
    mathvm::Translator* translator = mathvm::Translator::create(impl);

    const char* expr = "double x; double y;"
            "x += 8.0; y = 2.0;"
            "print('Hello, x=',x,' y=',y,'\n');"
    ;
    bool isDefaultExpr = true;

    if (script != NULL) {
        expr = mathvm::loadFile(script);
        if (expr == 0) {
            printf("Cannot read file: %s\n", script);
            return 1;
        }
        isDefaultExpr = false;
    }
    mathvm::Code* code = nullptr;

    mathvm::Status* translateStatus = translator->translate(expr, &code);
    if (translateStatus->isError()) {
        uint32_t position = translateStatus->getPosition();
        uint32_t line = 0, offset = 0;
        mathvm::positionToLineOffset(expr, position, line, offset);
        printf("Cannot translate expression: expression at %d,%d; "
                       "error '%s'\n",
               line, offset,
               translateStatus->getError().c_str());
    } else {
        if (code) {
            code->disassemble();
        }
    }
    delete translateStatus;
    delete translator;

    if (!isDefaultExpr) {
        delete [] expr;
    }
    return 0;
}

int test_inerpreter(int argc, char** argv) {
    std::string impl = "translator";
//    const char* script = nullptr;
    const char* script = nullptr;
    for (int32_t i = 1; i < argc; i++) {
        if (std::string(argv[i]) == "-j") {
            impl = "jit";
        } else {
            script = argv[i];
        }
    }
    // script = "/home/rauf/Programs/semester_3/vm/mathvm/students/2016/kurbanov/bytecode.txt";
    TranslatorPtr translator(mathvm::Translator::create(impl));
    // TODO place usage here
    if (!translator) {
        return 1;
    }


    // TODO place usage here
    if (script == nullptr) {
        printf("No script name\n");
        return 1;
    }

    const char* expr = mathvm::loadFile(script);
    // TODO place usage here
    if (expr == 0) {
        printf("Cannot load file: %s\n", script);
//        std::cout << "Cannot load file: %s\n" << script;
        return 1;
    }

    mathvm::Code* code = nullptr;
    StatusPtr translateStatus(translator->translate(expr, &code));
    if (translateStatus->isError()) {
        uint32_t position = translateStatus->getPosition();
        uint32_t line = 0, offset = 0;
        mathvm::positionToLineOffset(expr, position, line, offset);
        printf("Cannot translate expression: expression at %d,%d; "
                       "error '%s'\n",
               line, offset,
               translateStatus->getError().c_str());
        return 1;

    } else {
        if (code) {
            Interpreter interpreter(code);
            try {
                interpreter.execute();
            } catch(ExecutionException e) {
                std::cout << e.what() << std::endl;
                return 1;
            }
        } else {
            std::cout << "No code to interpret" << std::endl;
            return 1;
        }
    }

    delete[] expr;
    delete code;

    return 0;
}

int main(int argc, char** argv) {
//    return test_printer(argc, argv);
//    return test_translator(argc, argv);
    return test_inerpreter(argc, argv);
}
