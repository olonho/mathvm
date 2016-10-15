//
// Created by user on 10/14/16.
//

#ifndef VM_STRINGUTILS_H
#define VM_STRINGUTILS_H

#include <string>

namespace utils {
    using namespace std;

    class StringUtils {
    public:
        static string escapeString(const string &str) {
            string result = "";
            result.reserve(str.size());
            for (auto it = str.begin(); it != str.end(); ++it) {
                switch (*it) {
                    case '\n':
                        result.push_back('\\');
                        result.push_back('n');
                        break;
                    case '\t':
                        result.push_back('\\');
                        result.push_back('t');
                        break;
                    case '\b':
                        result.push_back('\\');
                        result.push_back('b');
                        break;
                    case '\r':
                        result.push_back('\\');
                        result.push_back('r');
                        break;
                    default:
                        result.push_back(*it);
                }
            }
            return result;
        }
    };
};


#endif //VM_STRINGUTILS_H
