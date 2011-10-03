#include <map>
#include <string>

using std::map;
using std::string;

class VarTable {
	map<string, uint8> _vars;
	uint8_t _counter;
	public:
		VarTable(): 
			_counter(0) 
		{}
		uint8_t getIdByName(const string& varName);
		uint8_t addVar(const string& varName);
	
};
