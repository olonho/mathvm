#ifndef TRANSLATIONEXCEPTION_H
#define TRANSLATIONEXCEPTION_H

#include <string>

class TranslationException {
private:
    std::string myMessage;
public:
    TranslationException(std::string const& message) : myMessage(message) {
    }

    virtual std::string what() const {
        return myMessage;
    }
};

#endif // TRANSLATIONEXCEPTION_H
