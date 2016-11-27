#ifndef INTERPRETEREXCEPTION_H
#define INTERPRETEREXCEPTION_H

namespace mathvm
{
    class InterpreterException : public std::exception 
    {
    public:
        InterpreterException(const std::string& message) 
            : _msg(message.c_str()) 
        {}

        InterpreterException(const char* message) 
            : _msg(message) 
        {}

        char const* what() const throw() override
        { 
            return _msg;
        }

    private:
        char const* const _msg;
    };
}

#endif /* INTERPRETEREXCEPTION_H */

