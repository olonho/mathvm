#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include <stdexcept>
#include <string>

using std::string;

class Exception : public std::exception
{
	string msg;
  public:
	Exception(string const & message) : msg(message) {}

	const char * what() const throw()
	{
		return msg.c_str();
	}
	~Exception() throw () {}
};


class TranslatorException : public Exception
{
  public:
	explicit TranslatorException(string const & message) : Exception("you have error, while translation " + message) {}
};

class InterpretationException : public Exception
{
  public:
	explicit InterpretationException(string const & message) : Exception("you have error, while interpretation " + message) {}
};

#endif // EXCEPTIONS_H
