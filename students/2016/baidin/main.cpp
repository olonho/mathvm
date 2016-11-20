#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>

#include "mathvm.h"
#include "parser.h"
#include "BytecodeTranslator.h"
#include "BytecodeInterpreter.h"
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>


#include <iostream>

namespace mathvm {
    using namespace std;

    class ToBytecodeTranslator : public Translator {
        ostream &out;
    public:
        ToBytecodeTranslator(ostream &out = cout) : out(out) {};

        virtual ~ToBytecodeTranslator() override {}

        virtual Status *translate(const string &program, Code **code) override {
            Parser parser = Parser();
            Status *parseStatus = parser.parseProgram(program);
            if (parseStatus->isError()) {
                return parseStatus;
            }
            delete parseStatus;


            BytecodeInterpreter *interpreter = new BytecodeInterpreter(out);
            try {
                BytecodeFunction *topFunction = new BytecodeFunction(parser.top());
                interpreter->addFunction(topFunction);
                ToBytecodeVisitor *visitor = new ToBytecodeVisitor(interpreter, topFunction->bytecode(),
                                                                   topFunction);
                parser.top()->node()->visit(visitor);
                delete visitor;
            } catch (const char *msg) {
                return Status::Error(msg);
            }

            *code = interpreter;

            return Status::Ok();
        }
    };


    Translator *Translator::create(const string &impl) {
        return new ToBytecodeTranslator();
    }
}


using namespace mathvm;
using namespace std;


void translateAndRun(string &programText, Translator *translator) {
    Code *code = 0;

    Status *translateStatus = translator->translate(programText, &code);
    if (translateStatus->isError()) {
        uint32_t position = translateStatus->getPosition();
        uint32_t line = 0, offset = 0;
        positionToLineOffset(programText, position, line, offset);
        printf("Cannot translate expression: expression at %d,%d; "
                       "error '%s'\n",
               line, offset,
               translateStatus->getErrorCstr());
    } else {
        vector<Var *> vars;


        Status *execStatus = code->execute(vars);
        if (execStatus->isError()) {
            printf("Cannot execute expression: error: %s\n",
                   execStatus->getErrorCstr());
        }
        delete code;
        delete execStatus;
    }
    delete translateStatus;
    delete translator;
}

string loadText(const string &fileName) {
    ifstream inputFileStream{fileName};

    stringstream stringStream;
    stringStream << inputFileStream.rdbuf();

    return stringStream.str();
}


int main(int argc, char **argv) {
    string impl = "";
    if (argc <= 1) {
        cout << "No file name arg!" << endl;
        return 1;
    }

    //test mode
    if (string(argv[1]) == "-t") {
        vector<string> tests = {"ackermann", "add", "assign", "bitwise", "div", "expr", "for", "function", "if", "literal", "mul",
                                "sub", "while"};
        for (auto it = tests.begin(); it != tests.end(); ++it) {
            stringstream stream;
            Translator *translator = new ToBytecodeTranslator{stream};
            cout << "test name:" << (*it) << endl;
            string mvmFileName = "./tests/" + (*it) + ".mvm";
//            cout << "load from " << mvmFileName << endl;

            string programText = loadText(mvmFileName.c_str());

//            cout << programText << endl;
            translateAndRun(programText, translator);
            string resultText = stream.str();
//            cout << "result: \n" << resultText << endl;

            string expectedFileName = "./tests/" + (*it) + ".expect";
            string expectedString = loadText(expectedFileName.c_str());
//            cout << "expected: \n" << expectedString << endl;

            if (resultText != expectedString) {
                cout << "TEST FAILED!" << endl;
            } else {
                cout << "TEST PASSED!" << endl;
            }
        }
        return 0;
    }

    string programText = loadText(argv[1]);

    Translator *translator = Translator::create(impl);
    translateAndRun(programText, translator);

    return 0;
}