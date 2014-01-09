#include "StackLayout.h"

#include "VarsUtil.h"

uint32_t mixTypeAndOffset(int type, int16_t offset)
{
	return static_cast<uint32_t>(type) << 30 | (static_cast<uint32_t>(offset) << 16 >> 16);
}

StackLayout::StackLayout(const Signature& _signature, const string& _name, int16_t _id,
		const set<VarID>& _captured) : signature_(_signature)
{
	int16_t offset = -16;

	for (size_t i = signature_.size() - 1; i > 0; --i)
	{
		offset -= sizeOfType(signature_[i].first);
		offsets_.push_front(make_pair(signature_[i], offset));
	}

	set<VarID>::const_iterator it;
	for (it = _captured.begin(); it != _captured.end(); ++it)
	{
		offset -= PointerSize;
		capturedOffsets_.push_front(make_pair(*it, offset));
	}
}


StackLayout::StackLayout()
{
}


void StackLayout::addLocalVars(Scope* scope)
{
	Scope::VarIterator varIt = scope;
	int16_t offset = 0;
	int16_t lastOffset = 0;
	if (!offsets_.empty())
		lastOffset += sizeOfType(offsets_.back().first.first) + offsets_.back().second;

	if (lastOffset > offset)
		offset = lastOffset;

	while(varIt.hasNext())
	{
		AstVar* var = varIt.next();
		offsets_.push_back(make_pair(make_pair(var->type(), var->name()), offset));
		offset += sizeOfType(var->type());
	}
}


void StackLayout::remLocalVars(int count)
{
	list<VarOffset>::iterator it = offsets_.end();
	advance(it, -count);
	offsets_.erase(it, offsets_.end());
}


void StackLayout::pushLocalVar(VarID var)
{
	int16_t offset = 0;
	int16_t lastOffset = 0;
	if (!offsets_.empty())
		lastOffset += sizeOfType(offsets_.back().first.first) + offsets_.back().second;

	if (lastOffset > offset)
		offset = lastOffset;

	offsets_.push_back(make_pair(var, offset));
}


void StackLayout::popLocalVar()
{
	if (!offsets_.empty())
		offsets_.pop_back();
}


uint32_t StackLayout::getLoadOffsetAsPtr(VarID var) const
{
	list<VarOffset>::const_reverse_iterator it = offsets_.rbegin();
	while (it != offsets_.rend() && it->first != var)
		++it;
	if (it != offsets_.rend())
		return mixTypeAndOffset(1, it->second);

	it = capturedOffsets_.rbegin();
	while (it != capturedOffsets_.rend() && it->first != var)
		++it;
	if (it != capturedOffsets_.rend())
		return mixTypeAndOffset(3, it->second);

	return 0;
}


uint32_t StackLayout::getLoadOffsetAsVal(VarID var) const
{
	list<VarOffset>::const_reverse_iterator it = offsets_.rbegin();
	while (it != offsets_.rend() && it->first != var)
		++it;
	if (it != offsets_.rend())
		return mixTypeAndOffset(0, it->second);

	it = capturedOffsets_.rbegin();
	while (it != capturedOffsets_.rend() && it->first != var)
		++it;
	if (it != capturedOffsets_.rend())
		return mixTypeAndOffset(2, it->second);

	return 0;
}


uint32_t StackLayout::getStoreOffset(VarID var) const
{
	list<VarOffset>::const_reverse_iterator it = offsets_.rbegin();
	while (it != offsets_.rend() && it->first != var)
		++it;
	if (it != offsets_.rend())
		return 0 << 30 | it->second;

	it = capturedOffsets_.rbegin();
	while (it != capturedOffsets_.rend() && it->first != var)
		++it;
	if (it != capturedOffsets_.rend())
		return 3 << 30 | it->second;

	return 0;
}


const VarID* StackLayout::first() const
{
	return atFromBack(0);
}


const VarID* StackLayout::second() const
{
	return atFromBack(1);
}


const Signature& StackLayout::signature() const
{
	return signature_;
}


const VarID* StackLayout::atFromBack(size_t idx) const
{
	if (offsets_.size() > idx)
	{
		list<VarOffset>::const_reverse_iterator it = offsets_.rbegin();
		advance(it, idx);
		if (it != offsets_.rend())
			return &(it->first);
	}
	return 0;
}


bool operator==(VarID vid1, VarID vid2)
{
	return (vid1.first == vid2.first && vid1.second.compare(vid2.second) == 0);
}


ostream& operator<<(ostream& os, VarOffset v)
{
	os << v.first.first << "\t" << v.first.second << "\t" << v.second;
	return os;
}
