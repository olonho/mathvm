#pragma once

#include <exception>
#include <string>

namespace mathvm {
    using std :: exception;
    using std :: string;
    namespace details {
        struct TranslationException : public exception {
            TranslationException(const string& message);
            TranslationException(const char* message);
            virtual const char* what() const noexcept;
        private:
            const char* _message;
        };
    } //end namespace details
} // end namespace mathvm
