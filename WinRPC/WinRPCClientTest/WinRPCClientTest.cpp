// WinRPCClientTest.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"
#include "../WinRPC/Manager/ChannelManager.h"
#pragma comment(lib, "../Debug/WinRPC.lib")

int _tmain(int argc, _TCHAR* argv[])
{
	ChannelManager manager;
	const std::string channelName = "hkxiaoyu118";
	if (manager.AddChannel(channelName, false) == true)
	{
		std::cout << "���ͨ���ɹ�" << std::endl;
	}
	else
	{
		std::cout << "���ͨ��ʧ��" << std::endl;
	}
	while (true)
	{
		std::string data = "this is a test";
		manager.SendData(channelName, data);
	}
	system("pause");
	return 0;
}

