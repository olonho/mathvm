/*
 * MVException.cpp
 *
 *  Created on: 18.01.2012
 *      Author: Pavel Sinay
 */

#include "MVException.h"
#include <fstream>
#include <sstream>

std::string intToStr(int i) {
	std::ostringstream os;
	os << i;
	return os.str();
}

int strToInt(std::string str) {
	std::stringstream stream;
	stream << str;
	int i;
	stream >> i;
	return i;
}

MVException::MVException(const std::string & message) :
	m_message(message), m_position(0) {
}

MVException::MVException(const std::string &message, int position) :
	m_message(message), m_position(position) {

}

MVException::~MVException() throw () {
}

const char* MVException::what() const throw () {
	return m_message.c_str();
}

int MVException::getPosition() const {
	return m_position;
}

