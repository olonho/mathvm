#ifndef UTILITIES_H
#define	UTILITIES_H
#include "ast.h"
#include "parser.h"
#include <map> 
#include <exception>
#include <sstream>

namespace mathvm {
    using std::map;
    
    typedef map<CustomDataHolder*, VarType> AstTypeMap;
    
    
    class ExceptionInfo : public std::exception {
    private:
        string msg;
    public :
        ExceptionInfo(string message): msg(message) { }
        
        ExceptionInfo() : msg("") { }
      
        virtual ~ExceptionInfo() throw(){};
      
        virtual const char * what() const throw() {
            return msg.c_str();
        }
      
        template<typename T>
        ExceptionInfo& operator<<( const T& value ) {
            stringstream stream;
            stream << value;
            msg += stream.str();
            return *this;
        }
      
    };
    
    struct ContextInfo {
        ContextInfo(const uint16_t id, const uint16_t functionId) : id(id), context(functionId) { }
        const uint16_t id;
        const uint16_t context;
    };
    
    
}  

#endif	/* UTILITIES_H */

