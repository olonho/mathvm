#ifndef AST2SRC_UTILS_HPP_
#define AST2SRC_UTILS_HPP_

#include <iterator>

template <class Iterator, class OutIterator>
void escape_all(Iterator begin, Iterator end, OutIterator out) {
	for (Iterator it = begin; it != end; ++it) {
		switch (*it) {
			case '\a': 
				*out++ = '\\';
				*out++ = 'a';
				break;
			case '\b': 
				*out++ = '\\';
				*out++ = 'b';
				break;
			case '\t': 
				*out++ = '\\';
				*out++ = 't';
				break;
			case '\n': 
				*out++ = '\\';
				*out++ = 'n';
				break;
			case '\v': 
				*out++ = '\\';
				*out++ = 'v';
				break;
			case '\f': 
				*out++ = '\\';
				*out++ = 'f';
				break;
			case '\r': 
				*out++ = '\\';
				*out++ = 'r';
				break;
			case '\\': 
				*out++ = '\\';
				*out++ = '\\';
				break;
			case '\"': 
				*out++ = '\\';
				*out++ = '\"';
				break;
			default:
				*out++ = *it;
		}

	}
}

std::string escape_all(const std::string &str) {
	std::string tmp;
	tmp.reserve(str.size() + str.size()/2);
	escape_all(str.begin(), str.end(), std::back_inserter(tmp));
	return tmp;
}

#endif // AST2SRC_UTILS_HPP_
