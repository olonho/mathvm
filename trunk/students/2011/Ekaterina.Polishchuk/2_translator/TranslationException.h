#ifndef TRANSLATIONEXCEPTION_H
#define TRANSLATIONEXCEPTION_H

class TranslationException {
public:
    TranslationException(std::string const& message) : myMessage(message) {}

    virtual std::string what() const {
        return myMessage;
    }
private:
    std::string myMessage;
};

#endif // TRANSLATIONEXCEPTION_H
