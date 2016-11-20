#ifndef GENERATOR_EXCEPTION_H
#define GENERATOR_EXCEPTION_H

namespace mathvm
{
    class GeneratorException : public std::exception 
    {
    public:
        GeneratorException(const std::string& message) 
            : _msg(message.c_str()) 
        {}

        GeneratorException(const char* message) 
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

#endif /* GENERATOR_EXCEPTION_H */

