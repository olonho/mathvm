#include "translation_exception.h"

#include <string>

namespace mathvm {
    using std :: string;
    namespace details {
        TranslationException :: TranslationException(const string& message)
            : _message(message.c_str()) {}

        TranslationException :: TranslationException(const char* message)
            : _message(message) {}

        const char* TranslationException :: what() const noexcept {
            return _message;
        }
    } // end namespace details
} // end namespace mathvm
