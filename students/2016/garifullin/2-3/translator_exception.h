#ifndef TRANSLATOR_EXCEPTION_H
#define TRANSLATOR_EXCEPTION_H

#include <exception>
#include <string>

namespace mathvm
{

class TranslatorException: public std::exception
{
public:
    explicit TranslatorException(char const *msg):
        m_msg(msg)
    {
    }

    explicit TranslatorException(std::string const &msg):
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

#endif // TRANSLATOR_EXCEPTION_H
