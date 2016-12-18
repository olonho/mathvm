#pragma once

#include <exception>
#include <mathvm.h>

class TranslationException : public std::exception {
public:
    TranslationException(mathvm::Status *status)
            : mStatus(status) {}

    const char *what() const noexcept { return mStatus->getError().c_str(); }

    mathvm::Status *errorStatus() { return mStatus; }

private:
    mathvm::Status *mStatus;
};
