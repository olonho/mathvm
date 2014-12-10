#ifndef TRANSLATION_EXCEPTION_H__
#define TRANSLATION_EXCEPTION_H__

#include <exception>

class TranslationException: public std::exception {
  public:
    TranslationException(Status* status): mStatus(status) {}

    const char* what() const noexcept {return mStatus->getError().c_str();}

    Status* errorStatus() {return mStatus;}

  private:
    Status* mStatus;
};

#endif
