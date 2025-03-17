#pragma once

#include <string>
#include <mutex>
#include <utility>
#include <Windows.h>

class RingBuffer
{
public:
	RingBuffer()
	{
		m_pData = nullptr;
		m_Capacity = 0;
		m_ReadPos = 0;
		m_WritePos = 0;
	}

	~RingBuffer()
	{
		if (m_pData != nullptr)
		{
			delete[] m_pData;
		}
	}

	bool Init(const int capacity_)
	{
		if (capacity_ <= 0)
		{
			return false;
		}

		m_Capacity = capacity_;

		if (m_pData != nullptr)
		{
			delete m_pData;
		}
		m_pData = new char[capacity_];
		ZeroMemory(m_pData, capacity_);

		return true;
	}


	bool enqueue(char* in_, unsigned int size_)
	{
		// 데이터 안담음
		if (size_ <= 0)
		{
			return false;
		}

		// 애초에 못담음
		if (size_ >= m_Capacity)
		{
			return false;
		}

		//std::lock_guard<std::mutex> guard(m_mutex);

		// 남은 공간 초과
		if ((m_WritePos > m_ReadPos && (m_WritePos + size_) >= m_ReadPos + m_Capacity) ||
			(m_WritePos < m_ReadPos && (m_WritePos + size_) >= m_ReadPos))
		{
			return false;
		}

		char* source = in_;
		int leftsize = size_;

		// 저장할 데이터가 선형버퍼의 끝에 도달함 -> 분할 저장해야함 (끝부분, 처음부분으로 나눠서)
		if (m_WritePos + size_ >= m_Capacity)
		{
			int availableSpace = m_Capacity - m_WritePos;
			CopyMemory(&m_pData[m_WritePos], source, availableSpace);
			m_WritePos = 0;
			source += availableSpace;
			leftsize -= availableSpace;
		}

		CopyMemory(&m_pData[m_WritePos], source, leftsize);
		m_WritePos += leftsize;

		m_WritePos %= m_Capacity;

		return true;
	}

	// ret : peeked Data size
	unsigned int dequeue(char* out_, unsigned int maxSize_)
	{
		//std::lock_guard<std::mutex> guard(m_mutex);

		if (IsEmpty())
		{
			return 0;
		}

		int remain;

		// 일반적인 경우
		if (m_WritePos > m_ReadPos)
		{
			remain = m_WritePos - m_ReadPos;
			if (remain > maxSize_)
			{
				remain = maxSize_;
			}

			CopyMemory(out_, &m_pData[m_ReadPos], remain);
			m_ReadPos += remain;
			m_ReadPos %= m_Capacity;
			return remain;
		}
		// 선형버퍼의 끝까지 읽으면 됨
		else if (m_WritePos == 0)
		{
			remain = m_Capacity - m_ReadPos;
			if (remain > maxSize_)
			{
				remain = maxSize_;
			}
			CopyMemory(out_, &m_pData[m_ReadPos], remain);
			m_ReadPos += remain;
			m_ReadPos %= m_Capacity;
			return remain;
		}
		// 선형버퍼의 끝까지 읽고 선형버퍼의 맨 앞부터 이어서 읽으면 됨
		else
		{
			remain = m_Capacity - m_ReadPos;

			if (remain > maxSize_)
			{
				remain = maxSize_;
				CopyMemory(out_, &m_pData[m_ReadPos], remain);
				m_ReadPos += remain;
				m_ReadPos %= m_Capacity;
				return remain;
			}

			CopyMemory(out_, &m_pData[m_ReadPos], remain);

			if (maxSize_ - remain < m_WritePos)
			{
				CopyMemory(out_ + remain, &m_pData[0], maxSize_ - remain);
				m_ReadPos = maxSize_ - remain;
				m_ReadPos %= m_Capacity;
				return maxSize_;
			}

			CopyMemory(out_ + remain, &m_pData[0], m_WritePos);
			m_ReadPos = m_WritePos;
			return remain + m_WritePos;
		}
	}

	unsigned int Size() const
	{
		if (IsEmpty())
		{
			return 0;
		}

		if (m_ReadPos < m_WritePos)
		{
			return m_WritePos - m_ReadPos;
		}

		return m_WritePos + m_Capacity - m_ReadPos;
	}
	bool IsEmpty() const { return m_ReadPos == m_WritePos; }
	bool IsFull() const { return (m_WritePos + 1) % m_Capacity == m_ReadPos; }
private:
	unsigned int m_ReadPos;
	unsigned int m_WritePos;

	char* m_pData;
	unsigned int m_Capacity;

	//std::mutex m_mutex;
};