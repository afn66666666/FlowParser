#include "pch.h"
#include "ReceiverImpl/ReceiverImpl.h"
#include "Interfaces/ICallback.h"

//used for debug

class Packages {
public:
	void addTextPackage(size_t size) {
		for (auto i = 0; i <
			size; ++i) {
			char rnd = 60 + rand() % 62;
			_data.push_back(rnd);
		}
		_packages.emplace_back(_data.end() - size, _data.end());
		_data.push_back('\r');
		_data.push_back('\n');
		_data.push_back('\r');
		_data.push_back('\n');
	}
	void addBinPackage(uint32_t size) {
		_data.push_back(0x24);
		char a1 = size;
		char a2 = size >> 8;
		char a3 = size >> 16;
		char a4 = size >> 24;
		StorageIterator it = _data.end();
		_data.push_back(a1);
		_data.push_back(a2);
		_data.push_back(a3);
		_data.push_back(a4);
		for (auto i = 0; i <
			size; ++i) {
			char rnd = 60 + rand() % 62;
			_data.push_back(rnd);
		}
		_packages.emplace_back(_data.end() - size, _data.end());
	}

	std::vector<char> getPackageData(int packageNum) {
		return _packages[packageNum];
	}
	std::vector<char> _data;
	std::vector<std::vector<char>> _packages;
};


class CB : public ICallback {
public:
	virtual void BinaryPacket(const char* data, std::size_t size) override {
		_results.emplace_back(data,data+size);

	};
	virtual void TextPacket(const char* data, std::size_t size) override {
		std::vector<char> out2{ data,data + size };
		_results.push_back(std::move(out2));

	};

	std::vector<std::vector<char>> _results;
};

void assertPackages(const std::vector<std::vector<char>>& v1, const std::vector<std::vector<char>>& v2) {
	if (v1.size() == v2.size()) {
		for (int i = 0; i < v1.size(); ++i) {
			EXPECT_EQ(std::string(v1[i].data(), v1[i].size()),
				std::string(v2[i].data(), v2[i].size()));
		}
	}
	EXPECT_TRUE(v1.size(), v2.size());
	
}


TEST(ReceiverImplTests, t1) {
	auto bl = new Packages();
	auto cb = new CB();
	auto receiver = new ReceiverImpl(cb);
	bl->addBinPackage(32);
	receiver->Receive(bl->_data.data(), bl->_data.size());
	assertPackages(bl->_packages, cb->_results);
}

TEST(ReceiverImplTests, t2) {
	auto bl = new Packages();
	auto cb = new CB();
	auto receiver = new ReceiverImpl(cb);
	bl->addTextPackage(32);
	receiver->Receive(bl->_data.data(), bl->_data.size());
	assertPackages(bl->_packages, cb->_results);
}

TEST(ReceiverImplTests, t3) {
	auto bl = new Packages();
	auto cb = new CB();
	auto receiver = new ReceiverImpl(cb);
	bl->addTextPackage(32);
	bl->addTextPackage(132);
	receiver->Receive(bl->_data.data(), 36);
	receiver->Receive(bl->_data.data()+36, 132);
	assertPackages(bl->_packages, cb->_results);
}

TEST(ReceiverImplTests, t4) {
	auto bl = new Packages();
	auto cb = new CB();
	auto receiver = new ReceiverImpl(cb);
	bl->addBinPackage(32);
	bl->addTextPackage(11);
	bl->addBinPackage(44);
	bl->addTextPackage(22);
	receiver->Receive(bl->_data.data(), bl->_data.size());
	assertPackages(bl->_packages, cb->_results);
}

TEST(ReceiverImplTests, t5) {
	auto bl = new Packages();
	auto cb = new CB();
	auto receiver = new ReceiverImpl(cb);
	bl->addBinPackage(500);
	bl->addBinPackage(440);
	receiver->Receive(bl->_data.data(), 1);
	receiver->Receive(bl->_data.data()+1, 2);
	receiver->Receive(bl->_data.data()+3, 3);
	receiver->Receive(bl->_data.data()+6, 500);
	receiver->Receive(bl->_data.data()+506, 2);
	receiver->Receive(bl->_data.data()+508, 442);
	assertPackages(bl->_packages, cb->_results);
}

TEST(ReceiverImplTests, t6) {
	auto bl = new Packages();
	auto cb = new CB();
	auto receiver = new ReceiverImpl(cb);
	bl->addTextPackage(500);
	bl->addTextPackage(440);
	receiver->Receive(bl->_data.data(), 1);
	receiver->Receive(bl->_data.data() + 1, 2);
	receiver->Receive(bl->_data.data() + 3, 3);
	receiver->Receive(bl->_data.data() + 6, 500);
	receiver->Receive(bl->_data.data() + 506, 2);
	receiver->Receive(bl->_data.data() + 508, 440);
	assertPackages(bl->_packages, cb->_results);
	//assertPackages(bl->_packages, cb->_results);
}

TEST(ReceiverImplTests, t7) {
	auto bl = new Packages();
	auto cb = new CB();
	auto receiver = new ReceiverImpl(cb);
	bl->addBinPackage(32);
	bl->addTextPackage(11);
	bl->addBinPackage(44);
	bl->addTextPackage(22);
	receiver->Receive(bl->_data.data(), 3);
	receiver->Receive(bl->_data.data() + 3, 39);
	receiver->Receive(bl->_data.data() + 42, 13);
	receiver->Receive(bl->_data.data() + 55, 47);
	receiver->Receive(bl->_data.data() + 55, 27);
	assertPackages(bl->_packages, cb->_results);
}