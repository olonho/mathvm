#include "mathvm.h"

#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <iostream> 

using namespace mathvm;
using namespace std;

char* loadFile(const char* file) {
    int fd = open(file, O_RDONLY);
    if (fd < 0) {
        return 0;
    }
    struct stat statBuf;
    if (fstat(fd, &statBuf) != 0) {
        return 0;
    }

    off_t size = statBuf.st_size;
    uint8_t* result = new uint8_t[size + 1];
    
    int rv = read(fd, result, size);
    assert(rv == size);
    result[size] = '\0';    

    return (char*)result;
}

void positionToLineOffset(const string& text,
                          uint32_t position, uint32_t& line, uint32_t& offset) {
    uint32_t pos = 0, max_pos = text.length();
    uint32_t current_line = 1, line_start = 0;
    while (pos < max_pos) {
        while (pos < max_pos && text[pos] != '\n') {
            pos++;
        }
        if (pos >= position) {
            line = current_line;
            offset = position - line_start;
            return;
        }
        assert(text[pos] == '\n' || pos == max_pos);
        pos++; // Skip newline.
        line_start = pos;
        current_line++;
    }
}

int main(int argc, char** argv) {
    Translator* translator = Translator::create();

    if (translator == 0) {
        cout << "TODO: Implement translator factory in translator.cpp!!!!" << endl;
        return 1;
    }

    const char* expr = "double x; double y;"
                        "x += 8.0; y = 2.0;" 
                        "print('Hello, x=',x,' y=',y,'\n');"
        ;
    bool isDefaultExpr = true;

    if (argc > 1) {
        expr = loadFile(argv[1]);
        if (expr == 0) {
            printf("Cannot read file: %s\n", argv[1]);
            return 1;
        }
        isDefaultExpr = false;
    }

    Code* code = 0;

    Status* translateStatus = translator->translate(expr, &code);
    if (translateStatus->isError()) {
        uint32_t position = translateStatus->getPosition();
        uint32_t line = 0, offset = 0;
        positionToLineOffset(expr, position, line, offset);
        printf("Cannot translate expression: expression at %d,%d; "
               "error '%s'\n",
               line, offset,
               translateStatus->getError().c_str());
#if 0
        if (position != Status::INVALID_POSITION) {
            printf("%s\n", expr);
            for (uint32_t i = 0; i < position; i++) {
                printf(" ");
            }
            printf("^\n");
        }
#endif
    } else {
        assert(code != 0);
        vector<Var*> vars;

        if (isDefaultExpr) {
            Var* xVar = new Var(VT_DOUBLE, "x");
            Var* yVar = new Var(VT_DOUBLE, "y");            
            vars.push_back(xVar);
            vars.push_back(yVar);
            xVar->setDoubleValue(42.0);
        }
        Status* execStatus = code->execute(vars);
        if (execStatus->isError()) {
            printf("Cannot execute expression: error: %s\n",
                   execStatus->getError().c_str());
        } else {
            if (isDefaultExpr) {
                printf("x evaluated to %f\n", vars[0]->getDoubleValue());
            }
        }
        delete code;
        delete execStatus;
    }
    delete translateStatus;
    delete translator;

    return 0;
}
