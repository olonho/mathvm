#pragma once

#include <stdexcept>


namespace mathvm {
    class Error : public std::logic_error {
    public:
        Error (string const &message)
                : std::logic_error(message) {
        }
    };


    class TranslationError : public Error {

    public:
        TranslationError(string const &message) : Error(message) {
        }
    };

    class ParseError : public Error {

    public:
        ParseError(string const &message) : Error(message) {
        }
    };

    class CodeGenerationError : public Error {

    public:
        CodeGenerationError(string const &message) : Error(message) {
        }
    };
    class BadIr : public Error {

    public:
        BadIr(string const &message) : Error(message) {
        }
    };

    class NotImplemented : public Error {

    public:
        NotImplemented(string const &message) : Error("Operation " + message + " is not yet supported") {
        }
    };
}