#ifndef __TRANSLATIONERROR_HPP_
#define __TRANSLATIONERROR_HPP_

#include <string>
#include <stdexcept>

namespace mathvm {

    class MathVMError : public std::logic_error {
    public:
        MathVMError(string const &message, uint32_t position = 0)
                : std::logic_error(message), position(position) {
        }

        uint32_t where() const {
            return position;
        }

    private:
        uint32_t position;
    };

    class InterpretationError : public MathVMError {
    public:
        InterpretationError(string const &message, uint32_t position = 0)
                : MathVMError(message, position) {
        }
    };

    class TranslationError : public MathVMError {
    public:
        TranslationError(string const &message, uint32_t position = 0)
                : MathVMError(message, position) {
        }
    };

} //mathvm

#endif
