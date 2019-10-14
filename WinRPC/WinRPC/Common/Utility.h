#pragma once
#include <vector>
#include <string>
#include <windows.h>

typedef void __stdcall FunProcessRecvData(const char* channelName, const char* data, unsigned int dataLength, void* pContext);

//����˵���ϸ��Ϣ
struct ClientNode
{
	std::string clientName;	//�ͻ��˵�����
	DWORD pid;				//�ͻ������ڽ��̵�PID
};

struct MsgNode
{
	std::string clientOrServerName;	//�ͻ��˵�����
	std::string data;				//���ݵ�����
};

struct ServerNode
{
	std::string serverName;	//����˵�����
	DWORD pid;				//��������ڽ��̵�PID
};

std::vector<std::string> StrSplit(std::string str, std::string pattern);