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

std::string intToStr(int i);
int strToInt(std::string str);

class MVException: public std::exception {
public:
	MVException(const std::string &message);
	MVException(const std::string &message, int position);
	virtual ~MVException() throw ();
	const char* what() const throw ();
	int getPosition() const;
private:
	std::string m_message;
	int m_position;
};

#endif /* MVEXCEPTION_H_ */
