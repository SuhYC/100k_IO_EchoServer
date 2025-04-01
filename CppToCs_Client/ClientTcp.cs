using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Net;
using System.Net.Sockets;

namespace CsMemcpyIOTest
{
    internal class ClientTcp
    {
        public const int MAX_SOCKBUF = 4096;

        private NetworkStream _stream;
        private TcpClient _tcpClient;
        private byte[] _buffer;

        private byte[] _SendingData;
        private RingBuffer _SendBuffer;

        private bool m_IsRun;

        public static ClientTcp Instance
        {
            get { return _instance; }
        }

        private static ClientTcp _instance;

        public static void Set(ClientTcp instance)
        {
            _instance = instance;
        }

        public void Restart()
        {
            if(_SendBuffer == null)
            {
                _SendBuffer = new RingBuffer();
            }
            else
            {
                _SendBuffer.Init();
            }

            if(_SendingData == null)
            {
                _SendingData = new byte[MAX_SOCKBUF];
                _buffer = new byte[MAX_SOCKBUF];
            }
            else
            {
                Array.Clear(_buffer, 0, MAX_SOCKBUF);
                Array.Clear(_SendingData, 0, MAX_SOCKBUF);
            }

            m_IsRun = true;
        }

        /// <summary>
        /// 서버 연결.
        /// 이후 서버응답수신함수RecvMsg()를 호출한다. RecvMsg는 연결이 끊어질 때까지 내부에서 반복.
        /// </summary>
        /// <param name="host"></param>
        /// <param name="port"></param>
        /// <returns></returns>
        public async Task ConnectToTcpServer(string host, int port)
        {
            try
            {
                _tcpClient = new TcpClient();
                await _tcpClient.ConnectAsync(host, port);
                _tcpClient.Client.NoDelay = true;
                _stream = _tcpClient.GetStream();
            }
            catch (Exception e)
            {
                Console.WriteLine($"NetworkManager::ConnectToTcpServer : 연결 오류. {e.Message}");
                return;
            }

            Console.WriteLine($"NetworkManager::ConnectToTcpServer : 연결 완료");

            Task recv = RecvMsg();

            Task send = SendIO();

            await Task.WhenAll(recv, send);

            return;
        }

        public void End()
        {
            m_IsRun = false;

            _stream?.Close();
            _tcpClient?.Close();
        }

        public void EnqueueSendData(string msg)
        {
            byte[] tmp = Encoding.GetEncoding("euc-kr").GetBytes(msg);

            uint size = (uint)tmp.Length;

            byte[] data = new byte[size + sizeof(uint)];

            data[0] = (byte)(size & 0xFF);
            data[1] = (byte)((size >> 8) & 0xFF);
            data[2] = (byte)((size >> 16) & 0xFF);
            data[3] = (byte)((size >> 24) & 0xFF);

            Buffer.BlockCopy(tmp, 0, data, sizeof(uint), tmp.Length);

            bool ret = _SendBuffer.Enqueue(data, (int)size + sizeof(uint));

            if (!ret)
            {
                Console.WriteLine($"Full!");
            }
        }

        public async Task SendMsg(byte[] msg, uint size_)
        {
            if (_tcpClient != null && _tcpClient.Connected)
            {
                byte[] data = new byte[size_ + sizeof(uint)];

                data[0] = (byte)(size_ & 0xFF);
                data[1] = (byte)((size_ >> 8) & 0xFF);
                data[2] = (byte)((size_ >> 16) & 0xFF);
                data[3] = (byte)((size_ >> 24) & 0xFF);

                Buffer.BlockCopy(msg, 0, data, sizeof(uint), (int)size_);

                await _SendBuffer.EnqueueWithLock(data, (int)size_ + sizeof(uint));
                //_SendBuffer.Enqueue(data, (int)size_ + sizeof(uint));
            }
        }

        public async Task SendIO()
        {
            while (true)
            {
                if(!m_IsRun)
                {
                    break;
                }

                int size = await _SendBuffer.DequeueWithLock(_SendingData);
                //int size = _SendBuffer.Dequeue(_SendingData);

                if (size == 0)
                {
                    await Task.Delay(0);
                    continue;
                }

                await _stream.WriteAsync(_SendingData, 0, size);
                //Array.Clear(_SendingData, 0, _SendingData.Length);
                //Console.WriteLine($"NetworkManager::SendIO : 데이터 전송: {_SendingData}, size : {_SendingData.Length}bytes.");
            }
        }

        /// <summary>
        /// 지속적으로 서버로부터의 응답을 체크.
        /// </summary>
        /// <returns></returns>
        private async Task RecvMsg()
        {
            try
            {
                while (true)
                {
                    if(!m_IsRun)
                    {
                        break;
                    }

                    int bytesRead = await _stream.ReadAsync(_buffer, 0, _buffer.Length);
                    if (bytesRead > 0)
                    {
                        //Console.WriteLine($"RecvMsg : {bytesRead}bytes Recved");
                        await ResHandler.Instance.HandlePacketByte(_buffer, bytesRead);
                    }
                    // 연결종료, 오류 발생
                    else
                    {
                        break;
                    }
                }
            }
            catch (Exception e)
            {
                Console.WriteLine($"NetworkManager::RecvMsg : {e.Message}");
            }
        }
    }
}
