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
		//std::cout << "RPGServer::OnReceive : RecvMsg : " << pData_  << ", size : " << ioSize_ << "\n";

		StoreMsg(index_, pData_, ioSize_);

		char buf[MAX_SOCKBUF] = { 0 };

		while (true)
		{
			ZeroMemory(buf, MAX_SOCKBUF);

			int len = GetMsg(index_, buf);

			//std::cout << "Server : len : " << len << '\n';

			if (len > MAX_SOCKBUF)
			{
				break;
			}

			if (len != 0)
			{
				//std::string str(buf);
				//std::cout << str << '\n';

				PacketData* pData = AllocatePacket();

				pData->Init(index_, buf, len);

				SendMsg(pData);
			}
			else
			{
				//std::cout << "Done!\n";
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