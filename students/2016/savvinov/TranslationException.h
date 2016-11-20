//
// Created by dsavvinov on 12.11.16.
//

#ifndef MATHVM_TRANSLATIONEXCEPTION_H
#define MATHVM_TRANSLATIONEXCEPTION_H


#include <exception>
#include <string>

namespace mathvm {
class TranslationException : public std::exception {
private:
    const char * message;
public:
    TranslationException(const char * message);
    virtual const char *what() const noexcept override;
};

class ExecutionException : public std::exception {
private:
    const char * message;
public:
    ExecutionException(const char * message);
    virtual const char *what() const noexcept override;
};
}
#endif //MATHVM_TRANSLATIONEXCEPTION_H
