#pragma once

/**
 * This class represents stack layout for each function
 * Stack:
 * bottom
 * ...
 * first captured variable pointer - bind and check on call
 * ...
 * first parameter variable			
 * ...
 * last parameter variable			<- -sizeof(last_variable)
 * stack pointer before call		<- base pointer 
 * instruction pointer before call	<- +4
 * first local variable				<- +8
 * ...
 *
 * last left bit = 0 - it's simple offset from base pointer to variable
 * else - it's offset to variable pointer
 * captured variable and string by pointer
 */

#include <ast.h>
using namespace mathvm;
#include <list>
using std::list;
#include <ostream>
using std::ostream;
#include <string>
using std::string;
#include <vector>
using std::vector;
#include <set>
using std::set;

typedef SignatureElement VarID;
typedef pair<VarID, int16_t> VarOffset;

bool operator==(VarID vid1, VarID vid2);
ostream& operator<<(ostream& os, VarOffset v);

/*
VT_STRING value is string* (!!!)
VT_STRING pointer is string** (!!!)

captured1 - by pointer
captured2 - by pointer
...
push first parameter - by value
...
eip
ebp
push local 1 - by value
push local 2

captured - always by pointer
local and signature - VT_DOUBLE, VT_INT by value; VT_STRING by pointer

first two bits for load:
0 - load by value
1 - get value   and load pointer
2 - get pointer and load value
3 - load pointer

first two bits for store
0 - replace value in stack
3 - replace value by pointer

getLoadOffsetAsPointer - use for load captured variable before call
if varible already pointer (in captured) - return 3_offset
if variable is value (in offsets) - return 1_offset

getLoadOffsetAsVariable - use for other loads
if variable (in captured) - pointer - return 2_offset
if variable (in offsets) - value - return 0_offset

getStoreOffset
if variable (in captured) - pointer - return 3_offset
if variable (in offsets)  - value - return 0_offset
*/

class StackLayout
{
public:
	/*
	*	signature is mandatory
	*/
	StackLayout(const Signature& _signature, const string& name, int16_t _id,
		const set<VarID>& _captured);
	StackLayout();

	void addLocalVars(Scope* scope);
	void remLocalVars(int count);

	void pushLocalVar(VarID var);
	void popLocalVar();

	uint32_t getLoadOffsetAsPtr(VarID var) const;
	uint32_t getLoadOffsetAsVal(VarID var) const;
	uint32_t getStoreOffset(VarID var) const;

	const VarID* first()  const;
	const VarID* second() const;

	const Signature& signature() const;

private:
	const VarID* atFromBack(size_t idx) const;

	list<VarOffset> capturedOffsets_;
	list<VarOffset> offsets_;

	Signature signature_;
};
