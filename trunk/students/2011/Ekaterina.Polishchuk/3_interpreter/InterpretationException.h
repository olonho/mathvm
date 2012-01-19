#ifndef INTERPRETATIONEXCEPTION_H
#define INTERPRETATIONEXCEPTION_H

class InterpretationException {
public:
    InterpretationException(std::string const& message) : myMessage(message) {
    }
    virtual std::string what() const {
        return myMessage;
    }
private:
    std::string myMessage;
};
#endif
