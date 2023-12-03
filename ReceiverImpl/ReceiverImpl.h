#pragma once
#include "Interfaces/IReceiver.h"
#include <vector>

using StorageIterator = std::vector<char>::iterator;
class ICallback;

struct __declspec(dllexport) ReceiverImpl : public IReceiver
{
	enum class BlockType {
		Binary,
		Text,
		BlockTypeCount
	};

public:
	ReceiverImpl(ICallback* callback);
	~ReceiverImpl() = default;
	void Receive(const char* data, std::size_t size) override;

private:
	/*!
	 * @brief indicate type of data block
	 * @param val first byte of block
	 * @return BlockType value
	*/
	BlockType			typeOfBlock(char val);

	/*!
	 * @brief main method for handling binary packages
	 * @param data - pointer to raw data of package
	 * @param bytesRemain - amount of not handled bytes in this block
	*/
	void				handleBinary(const char* data, uint32_t bytesRemain);

	/*!
	 * @brief main method for handling text packages
	 * @param data - pointer to raw data of package
	 * @param bytesRemain - amount of not handled bytes in this block
	*/
	void				handleText(const char* data, size_t bytesRemain);

	/*!
	 * @brief process of end sequence searching
	 * @param data - iterator pointing to possible end sequence
	 * @return iterator pointing to end of package. If end sequence was not founded, return storage.end()
	*/
	StorageIterator		sequenceAnalysis(StorageIterator data);

	/*!
	 * @brief handle possible end sequence, which not fits in current data block
	 * @param data 
	*/
	void				analyzeEndSequence(StorageIterator data);

	/*!
	 * @brief method tries to find ending sequence in begin of data block. It's calling if we suppose that last symbols in
	 * previous block (its saved in _collectedSequence) can be part of end sequence
	 * @param data - block data
	 * @param blockSize - size of block
	 * @return 
	*/
	bool				tryToEndPackage(const char* data, size_t blockSize);
	/*!
	 * @brief method tries to find meta bytes signals of binary's package size. Collected bytes puts in _sizeStorage
	 * @param data data block
	 * @param blockSize size of data block
	 * @return true - if _sizeStorage has defined size, otherwise - false
	*/
	bool				tryToCollectSize(const char* data, size_t blockSize);
	/*!
	 * @brief sends data in callback and prepare Receiver for next package
	 * @param data - package data
	 * @param size - package size
	*/
	void				sendPackage(const char* data, size_t size);

	bool				_readyToNewPackage{ true };
	ICallback*			_callback{};
	uint32_t			_currentPackageSize{ 0 };
	/*!
	 * @brief how many bytes was handled in current block
	*/
	size_t				_blockProgress{ 0 };
	BlockType			_packageType{ BlockType::BlockTypeCount };
	/*!
	 * @brief  iterator for correct inserting data in storage
	*/
	StorageIterator		_tracker;
	/*!
	 * @brief storage for packages which dont fit in data block. After sending data in callback, storage.clear() calls.
	*/
	std::vector<char>	_storage;
	/*!
	 * @brief buffer for collecting size of binary packages (in case of 4 bytes will be in different blocks
	*/
	std::vector<char>	_sizeStorage;
	/*!
	 * @brief buffer for collecting ending sequence of package
	*/
	std::string			_collectedSequence{};
};

