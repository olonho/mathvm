#ifndef INTERPRETER_ERROR_H
#define INTERPRETER_ERROR_H

#include <exception>
#include <string>

namespace mathvm
{

class InterpreterError: public std::exception
{
public:
    explicit InterpreterError(char const *msg):
        m_msg(msg)
    {
    }

    explicit InterpreterError(std::string const &msg):
        m_msg(msg.c_str())
    {
    }

    char const *what() const throw()
    {
        return m_msg.c_str();
    }

private:
    std::string m_msg;
};

}

#endif // INTERPRETER_ERROR_H
