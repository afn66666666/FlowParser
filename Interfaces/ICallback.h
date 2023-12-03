
#include "iostream"
struct ICallback
{
	virtual void BinaryPacket(const char* data, std::size_t size) = 0;
	virtual void TextPacket(const char* data, std::size_t size) = 0;
};