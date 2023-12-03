
#include "iostream"



struct IReceiver
{

	/*!
	 * @brief receive and handle data block, coming from flow
	 * @param data - pointer to block
	 * @param size - of data block 
	*/
	virtual void Receive(const char* data, std::size_t size) = 0;

	virtual ~IReceiver() = default;
};
