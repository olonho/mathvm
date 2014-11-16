#ifndef MY_INTERPRETER_HPP
#define MY_INTERPRETER_HPP

#include <iostream>
#include <ast.h>
#include <visitors.h>
#include <mathvm.h>
#include <parser.h>

using namespace mathvm;


class ICode: public Code {
public:
    Status *execute(std::vector<Var *> &vars);
};

#endif /* end of include guard: MY_INTERPRETER_HPP */
