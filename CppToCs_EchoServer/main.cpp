#include "Server.hpp"
#include <conio.h>
#include <Windows.h>

const int gBindPort = 12345;
const unsigned short gMaxClient = 100;

int main()
{
	Server server(gBindPort, gMaxClient);
	server.Start();
	std::cout << "�ƹ�Ű�� ������ ����˴ϴ�.\n";
	_getch();

	server.End();

}