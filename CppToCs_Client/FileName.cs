using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Diagnostics;

namespace CsMemcpyIOTest
{
    internal class FileName
    {
        static void Main()
        {
            const int port = 12345;
            const string serverIp = "127.0.0.1";

            const int PACKETNUM = 1000;

            ClientTcp test = new ClientTcp();
            ResHandler handler = new ResHandler();

            ClientTcp.Set(test);
            ResHandler.Set(handler);

            ResHandler.Instance.Init();

            test.Restart();

            for(int i = 0; i < PACKETNUM; i++)
            {
                test.EnqueueSendData("1");
            }

            Stopwatch sw = new Stopwatch();

            sw.Start();

            Task task = test.ConnectToTcpServer(serverIp, port);
            task.Wait();

            sw.Stop();

            Console.WriteLine($"Elapsed Time : {sw.ElapsedMilliseconds}ms.");
            Console.ReadKey();
        }
    }
}
