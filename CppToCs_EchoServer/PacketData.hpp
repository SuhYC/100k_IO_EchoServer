#pragma once

#include "Define.hpp"
#include <sstream>
#include <atomic>

// Network IO Packet
class PacketData
{
public:
	PacketData()
	{
		m_SessionIndex = 0;
	}

	virtual ~PacketData()
	{

	}

	// √ ±‚»≠
	void Init(const unsigned short sessionIndex_, char* pData_, DWORD size_)
	{
		m_SessionIndex = sessionIndex_;

		unsigned int size = size_;

		CopyMemory(pData, &size, sizeof(unsigned int));
		CopyMemory(pData + sizeof(unsigned int), pData_, size_);

		dataSize = size_ + sizeof(unsigned int);
	}

	char pData[MAX_SOCKBUF + sizeof(unsigned int)];
	unsigned int dataSize;
	unsigned short m_SessionIndex;
};