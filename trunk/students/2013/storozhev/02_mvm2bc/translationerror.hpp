#ifndef TRANSLATIONERROR_HPP
#define TRANSLATIONERROR_HPP

#include <string>
#include <stdexcept>

namespace mathvm {

class TranslationError : public std::logic_error
{
public:
    TranslationError(const std::string& message, uint32_t position = 0) :
        std::logic_error(message),
        m_pos(position)
    {    }

    uint32_t pos() const {
        return m_pos;
    }

private:
    uint32_t m_pos;
};

} //mathvm

#endif // TRANSLATIONERROR_HPP
