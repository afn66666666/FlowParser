#pragma once
#include "pch.h"
#include <vector>
#include <string>
#include <algorithm>

#include "Interfaces/ICallback.h"
#include "ReceiverImpl.h"

static const std::string endSequence = "\r\n\r\n";

ReceiverImpl::ReceiverImpl(ICallback* callback)
	:_callback{ callback }
{
	_tracker = _storage.begin();
}

void ReceiverImpl::Receive(const char* blockData, std::size_t blockSize)
{
	while (_blockProgress != blockSize) {
		if (_readyToNewPackage) {
			_packageType = typeOfBlock(*(blockData + _blockProgress));
			if (_packageType == BlockType::Binary) {
				++_blockProgress;
				if (blockSize - _blockProgress >= sizeof(uint32_t)) {
					_currentPackageSize = *(uint32_t*)(blockData + _blockProgress);
					_blockProgress += sizeof(uint32_t);
					handleBinary(blockData + _blockProgress, blockSize - _blockProgress);
				}
				else {
					_readyToNewPackage = false;
					_sizeStorage.insert(_sizeStorage.begin(), blockData + _blockProgress, blockData + blockSize);
					_blockProgress = blockSize;
				}
			}
			else {
				handleText(blockData + _blockProgress, blockSize - _blockProgress);
			}
		}
		else {
			if (_packageType == BlockType::Binary) {
				if (_currentPackageSize || tryToCollectSize(blockData, blockSize)) {
					handleBinary(blockData + _blockProgress, blockSize - _blockProgress);
				}
			}
			if (_packageType == BlockType::Text) {
				if (!_collectedSequence.empty()) {
					tryToEndPackage(blockData, blockSize);
					if (_collectedSequence.size() == endSequence.size()) {
						sendPackage(_storage.data(), _currentPackageSize);
					}
				}
				handleText(blockData + _blockProgress, blockSize - _blockProgress);
			}
		}
	}
	_blockProgress = 0;
}

ReceiverImpl::BlockType ReceiverImpl::typeOfBlock(char val)
{
	if (val == 0x24) {
		return BlockType::Binary;
	}
	return BlockType::Text;
}

void ReceiverImpl::handleBinary(const char* data, uint32_t size)
{
	if (_readyToNewPackage) {
		if (_currentPackageSize <= size) {
			_blockProgress += _currentPackageSize;
			sendPackage(data, _currentPackageSize);
		}
		else {
			//big package
			_readyToNewPackage = false;
			_storage.insert(_tracker, data, data + size);
			_tracker = _storage.end();
			_blockProgress += size;
		}
	}
	else {
		uint32_t remainSize = _currentPackageSize - _storage.size();
		if (remainSize <= size) {
			//end big package handle
			_storage.insert(_tracker, data, data + remainSize);
			_blockProgress += remainSize;
			sendPackage(_storage.data(), _currentPackageSize);

		}
		else {
			//continue big package				
			_storage.insert(_tracker, data, data + size);
			_tracker = _storage.end();
			_blockProgress += size;
		}
	}
}

void ReceiverImpl::handleText(const char* data, size_t blockSize)
{
	_tracker = _storage.insert(_tracker, data, data + blockSize);
	while (_tracker != _storage.end()) {
		auto candidate = std::find(_tracker, _storage.end(), '\r');
		_blockProgress += candidate - _tracker;
		_currentPackageSize += candidate - _tracker;
		if (candidate != _storage.end()) {
			_tracker = sequenceAnalysis(candidate);
			if (_tracker != _storage.end()) {
				_blockProgress += endSequence.size();
				_currentPackageSize = _tracker - _storage.begin();
				sendPackage(_storage.data(), _currentPackageSize);
				break;
			}
			else {
			}
		}
		else {
			_readyToNewPackage = false;
			_tracker = _storage.end();
		}
	}
}

StorageIterator ReceiverImpl::sequenceAnalysis(StorageIterator data)
{
	StorageIterator result = _storage.end();
	if (_storage.end() - data < 4) {
		analyzeEndSequence(data);
	}
	else {
		result = std::search(data, _storage.end(), endSequence.begin(), endSequence.end());
	}
	return result;
}

bool ReceiverImpl::tryToEndPackage(const char* data, size_t blockSize)
{
	auto bytesRemain = endSequence.size() - _collectedSequence.size();
	if (bytesRemain > blockSize) {
		for (auto i = 0; i < blockSize; ++i) {
			if (endSequence[_collectedSequence.size()] == data[i]) {
				_collectedSequence.push_back(data[i]);
				continue;
			}
			else {
				_collectedSequence.clear();
				return false;
			}
		}
		_blockProgress += blockSize;
		return true;
	}
	else {
		std::string buf{ data,data + bytesRemain };
		if (_collectedSequence + buf == endSequence) {
			_collectedSequence = endSequence;
			_blockProgress += bytesRemain;
			return true;
		}
	}
	return false;
}

bool ReceiverImpl::tryToCollectSize(const char* data, size_t blockSize)
{
	auto remainSize = sizeof(uint32_t) - _sizeStorage.size();
	if (remainSize > blockSize) {
		_sizeStorage.insert(_sizeStorage.end(), data, data + blockSize);
		_blockProgress = blockSize;
	}
	else {
		_sizeStorage.insert(_sizeStorage.end(), data, data + remainSize);
		_blockProgress += remainSize;
	}
	if (_sizeStorage.size() == sizeof(uint32_t)) {
		_currentPackageSize = *(uint32_t*)(_sizeStorage.data());
		_sizeStorage.clear();
		return true;
	}
	return false;
}

void ReceiverImpl::sendPackage(const char* data, size_t size)
{
	if (_packageType == BlockType::Binary) {
		_callback->BinaryPacket(data, size);
	}
	if (_packageType == BlockType::Text) {
		_callback->TextPacket(data, size);
	}
	_readyToNewPackage = true;
	_currentPackageSize = 0;
	_sizeStorage.clear();
	_storage.clear();
	_collectedSequence.clear();
	_tracker = _storage.begin();
}

void ReceiverImpl::analyzeEndSequence(StorageIterator data)
{
	_collectedSequence = std::move(std::string(data, _storage.end()));
	auto sequenceSize = _storage.end() - data;
	if (_collectedSequence != endSequence.substr(0, sequenceSize)) {
		_collectedSequence.clear();
	}
	_blockProgress += _storage.end() - data;
	_readyToNewPackage = false;
	_tracker = _storage.end();
}


