#pragma once

#include "IOCP.hpp"
#include <Windows.h>
#include <functional>
#include <stdexcept>

class Server : IOCPServer
{
public:
	Server(const int nBindPort_, const unsigned short MaxClient_) : m_BindPort(nBindPort_), m_MaxClient(MaxClient_)
	{
		InitSocket(nBindPort_);

	}

	~Server()
	{

	}

	void Start()
	{
		StartServer(m_MaxClient);
	}

	void End()
	{
		EndServer();
	}

private:
	void OnConnect(const unsigned short index_, const uint32_t ip_) override
	{
		std::cout << "RPGServer::OnConnect : [" << index_ << "] Connected.\n";
		return;
	}

	void OnReceive(const unsigned short index_, char* pData_, const DWORD ioSize_) override
	{
		//std::cout << "RPGServer::OnReceive : RecvMsg : " << pData_ << "\n";

		StoreMsg(index_, pData_, ioSize_);

		char buf[MAX_SOCKBUF] = { 0 };

		while (true)
		{
			unsigned int len = GetMsg(index_, buf);

			if (len != 0)
			{
				PacketData* pData = AllocatePacket();

				pData->Init(index_, buf, len);

				SendMsg(pData);
			}
			else
			{
				break;
			}
		}
		

		return;
	}

	void OnDisconnect(const unsigned short index_) override
	{
		std::cout << "RPGServer::OnDisconnect : [" << index_ << "] disconnected.\n";

		return;
	}

	const int m_BindPort;
	const unsigned short m_MaxClient;
};