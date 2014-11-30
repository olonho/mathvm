#ifndef EXCEPTIONS_HPP
#define EXCEPTIONS_HPP

#include <exception>


namespace mathvm
{

class BytecodeException: public std::exception
{
public:
    BytecodeException(char const *msg):
        m_msg(msg)
    {
    }

    char const *what() const throw()
    {
        return m_msg;
    }

private:
    char const * const m_msg;
};


class BytecodeGeneratorException: public BytecodeException
{
public:
    BytecodeGeneratorException(char const *msg, uint32_t position):
        BytecodeException(msg),
        m_position(position)
    {
    }

    uint32_t position() const
    {
        return m_position;
    }

private:
    uint32_t const m_position;
};

}


#endif // EXCEPTIONS_HPP
