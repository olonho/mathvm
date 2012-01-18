/*
 * MVException.cpp
 *
 *  Created on: 18.01.2012
 *      Author: Pavel Sinay
 */

#include "MVException.h"


MVException::MVException(const std::string & message) :
	m_message(message) {
	//std::cerr << "NetException: " << message << std::endl;
}

MVException::~MVException() throw () {
	// TODO Auto-generated destructor stub
}

const char* MVException::what() const throw () {
	return m_message.c_str();
}

