#pragma once

template <typename T>
class Flags
{
public:

	bool GetAny(T flags) { return (flags_ & flags) > 0; }
	bool GetAll(T flags) { return (flags_ & flags) == flags; }

	void Set(T flags) { flags_ |= flags; }
	void UnSet(T flags) { flags &= ~flags; }

private:
	T flags_;
};