/*
 * MVException.h
 *
 *  Created on: 18.01.2012
 *      Author: Pavel Sinay
 */

#ifndef MVEXCEPTION_H_
#define MVEXCEPTION_H_

#include <exception>
#include <string>

class MVException: public std::exception {
public:
	MVException(const std::string &message);
	virtual ~MVException() throw ();
	const char* what() const throw ();
private:
	std::string m_message;
};

#endif /* MVEXCEPTION_H_ */
