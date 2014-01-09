#ifndef INTERPRETATIONERROR_HPP
#define INTERPRETATIONERROR_HPP

#include <string>
#include <stdexcept>

namespace mathvm {

class InterpretationError : public std::logic_error
{
public:
    InterpretationError(const std::string& message, uint32_t position = 0) :
        std::logic_error(message),
        m_pos(position)
    {    }

    uint32_t pos() const {
        return m_pos;
    }

private:
    uint32_t m_pos;
};


} //namespace

#endif // INTERPRETATIONERROR_HPP
