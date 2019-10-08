// WinRPCServerTest.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"
#include <iostream>
#include "../WinRPC/Manager/ChannelManager.h"
#pragma comment(lib, "../Debug/WinRPC.lib")

int _tmain(int argc, _TCHAR* argv[])
{
	ChannelManager manager;
	const std::string channelName = "hkxiaoyu118";
	if (manager.AddChannel(channelName, true) == true)
	{
		std::cout << "���ͨ���ɹ�" << std::endl;
	}
	else
	{
		std::cout << "���ͨ��ʧ��" << std::endl;
	}

	unsigned int count = 0;
	while (true)
	{
		std::string data = "this is a server msg:" + std::to_string(count);
		manager.SendData(channelName, data);
		count++;
		Sleep(20);
	}
	system("pause");
	return 0;
}
