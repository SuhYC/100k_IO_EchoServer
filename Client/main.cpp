#pragma once

#define _CRT_SECURE_NO_WARNINGS

#include <WS2tcpip.h>
#include <WinSock2.h>
#include <iostream>
#include <mutex>
#include <atomic>
#include <sstream>

#include <string>
#include <thread>

#include <chrono>
#include <vector>

#pragma comment(lib, "ws2_32.lib")

#include "RingBuffer.hpp"

const int MAXSOCKBUF = 1024;
const int GOAL = 1000000;
const int PACKETNUM = 250;
const int RECV_THREAD_NUM = 1;

std::atomic_bool g_IsRun;
std::atomic_int cnt = 0;

std::atomic_int coutcnt = 0;

RingBuffer SendBuffer;
RingBuffer RecvBuffer;

SOCKET clientSocket;
std::mutex m;
std::mutex recvMutex;
std::mutex rbufferMutex;

SOCKET Init()
{
    WSADATA wsaData;
    int iRet = WSAStartup(MAKEWORD(2, 2), &wsaData);

    if (iRet != 0)
    {
        std::cerr << "WSAStartup failed\n";
        return INVALID_SOCKET;
    }

    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET)
    {
        std::cerr << "SOCKET creation falied\n";

        return INVALID_SOCKET;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(12345);
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

    iRet = connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));
    if (iRet == SOCKET_ERROR) {
        std::cerr << "Connect failed: " << WSAGetLastError() << std::endl;
        closesocket(clientSocket);
        WSACleanup();
        return INVALID_SOCKET;
    }

    return clientSocket;
}

int S_Deque(char* out_, size_t maxsize)
{
    std::lock_guard<std::mutex> guard(m);
    return SendBuffer.dequeue(out_, maxsize);
}

bool S_Enque(char* in_, size_t size)
{
    std::lock_guard<std::mutex> guard(m);
    return SendBuffer.enqueue(in_, size);
}

int R_Deque(char* out_, size_t maxsize)
{
    //std::lock_guard<std::mutex> guard(rbufferMutex);
    return RecvBuffer.dequeue(out_, maxsize);
}

bool R_Enque(char* in_, size_t size)
{
    //std::lock_guard<std::mutex> guard(rbufferMutex);
    return RecvBuffer.enqueue(in_, size);
}

char* pushbuf;

void Push(const char* pData_, unsigned int ioSize_)
{
    unsigned int len = ioSize_;

    if (ioSize_ > MAXSOCKBUF)
    {
        std::cerr << "TOO MUCH IOSIZE : " << ioSize_ << '\n';
        return;
    }

    CopyMemory(pushbuf, &len, sizeof(unsigned int));
    CopyMemory(pushbuf + sizeof(unsigned int), pData_, ioSize_);

    S_Enque(pushbuf, ioSize_ + sizeof(unsigned int));

    return;
}

unsigned int CheckStringSize(char* str_, unsigned int size_, char* out_)
{
    unsigned int header = 0;
    CopyMemory(&header, str_, sizeof(unsigned int));

    //std::cout << header << "bytes.\n";
    if (size_ < sizeof(unsigned int) + header)
    {
        //std::cout << size_ << "bytes. Not Enough.\n";
        return size_;
    }
    else
    {
        //std::cout << size_ << "bytes. Enough.\n";
        CopyMemory(out_, str_ + sizeof(unsigned int), header);
        CopyMemory(str_, str_ + header + sizeof(unsigned int), size_ - header - sizeof(unsigned int));
        return size_ - sizeof(unsigned int) - header;
    }
}

char* handleheaderbuf;

size_t HandleHeader(char* out_)
{
    //std::lock_guard<std::mutex> guard(rbufferMutex);

    int size = RecvBuffer.dequeue(handleheaderbuf, MAXSOCKBUF);

    if (size == 0)
    {
        return 0;
    }

    size_t remainlen = CheckStringSize(handleheaderbuf, size, out_);
    size = size - remainlen - sizeof(unsigned int);

    // 남은 데이터는 다시 저장한다.
    RecvBuffer.enqueue(handleheaderbuf, remainlen);

    return size;
}

void Send()
{
    char* buffer = new char[MAXSOCKBUF];

    while (g_IsRun)
    {
        int size = S_Deque(buffer, MAXSOCKBUF);

        if (size != 0)
        {
            int ret = send(clientSocket, buffer, size, 0);

            if (ret == SOCKET_ERROR)
            {
                std::cerr << "송신에러\n";
                break;
            }
            //ZeroMemory(buffer, MAXSOCKBUF);
        }
    }

    delete[] buffer;
}

int WaitRecv(char* buffer, int len)
{
    //std::lock_guard<std::mutex> guard(recvMutex);
    return recv(clientSocket, buffer, len, 0);
}

void Recv()
{
    char* buffer = new char[MAXSOCKBUF];

    while (g_IsRun)
    {
        int bytesRead = WaitRecv(buffer, MAXSOCKBUF);

        if (bytesRead > 0)
        {
            //RecvBuffer.enqueue(buffer, bytesRead);
            R_Enque(buffer, bytesRead);

            while (true)
            {
                size_t size = HandleHeader(buffer);

                if (size == 0)
                {
                    break;
                }
                else
                {
                    cnt++;

                    
                    if (cnt > GOAL / 100 * coutcnt)
                    {
                        coutcnt++;
                        std::cout << coutcnt << "%\n";
                    }

                    if (cnt.load() >= GOAL && g_IsRun == true)
                    {
                        g_IsRun = false;
                        closesocket(clientSocket);
                        break;
                    }
                }

                Push(buffer, size);
            }
        }
    }

    delete[] buffer;
}

void Test()
{
    g_IsRun = true;

    std::thread s_thread(Send);

    /*
    std::vector<std::thread> r_thread;

    for (int i = 0; i < RECV_THREAD_NUM; i++)
    {
        r_thread.emplace_back([]() {Recv(); });
    }*/

    std::thread r_thread(Recv);


    s_thread.join();

    r_thread.join();

    /*
    for (int i = 0; i < RECV_THREAD_NUM; i++)
    {
        r_thread[i].join();
        //std::cout << i << "Done.\n";
    }*/

    return;
}

int main()
{
    SendBuffer.Init(MAXSOCKBUF);
    RecvBuffer.Init(MAXSOCKBUF);

    clientSocket = Init();

    if (clientSocket == INVALID_SOCKET)
    {
        return 0;
    }

    pushbuf = new char[MAXSOCKBUF + sizeof(unsigned int)];
    handleheaderbuf = new char[MAXSOCKBUF];

    for (int i = 0; i < PACKETNUM; i++)
    {
        Push("1", sizeof("1"));
    }

    std::cout << "Start!\n";

    auto start = std::chrono::high_resolution_clock::now();
    Test();
    auto end = std::chrono::high_resolution_clock::now();

    std::cout << "Stop!\n";

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "Elapsed time : " << duration.count() << "msecs.";


    WSACleanup();

    delete[] pushbuf;
    delete[] handleheaderbuf;

    return 0;
}