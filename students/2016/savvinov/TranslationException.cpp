//
// Created by dsavvinov on 12.11.16.
//

#include "TranslationException.h"

namespace mathvm {
const char *TranslationException::what() const noexcept {
    return message;
}

TranslationException::TranslationException(const char *message) {
    this->message = message;
}

ExecutionException::ExecutionException(const char *message) {
    this->message = message;
}

const char *ExecutionException::what() const noexcept {
    return message;
}
}