#pragma once
#include <string>
#include <vector>
#include <queue>
#include <windows.h>
#include "../Common/ShareMemory.h"
#include "../Common/MyCriticalSection.h"

/*
	�ڴ�channel
*/

enum CHANNEL_ERROR
{
	NOT_ERROR,
	SHAREMEM_ERROR,
	SHAREADDR_ERROR,
	EVENT_ERROR
};

class MemoryChannel
{
public:
	MemoryChannel(const std::string channelName, bool isServer = false, DWORD shareMemorySize = 1024 * 4, unsigned sendMaxSize = 100, unsigned receiveMaxSize = 100);
	~MemoryChannel();
	CHANNEL_ERROR InitChannel();
	void StoreSendData(std::string data);
	bool GetSendData(std::string& data);
	void StoreReceiveData(std::string data); 
	bool GetReceiveData(std::queue<std::string>& dataSet);
	static unsigned  __stdcall SendDataThread(LPVOID args);
	static unsigned  __stdcall ReceiveDataThread(LPVOID args);


private:
	std::string m_channelName;			//ͨ������
	ShareMemory *m_shareMemoryClient;	//�ͻ��˹����ڴ�
	ShareMemory *m_shareMemoryServer;	//����˹����ڴ�
	void* m_shareMemoryClientAddr;		//�ͻ��˹����ڴ��ַ
	void* m_shareMemoryServerAddr;		//�����������ڴ��ַ
	HANDLE m_eventClientRead;			//�ͻ��˶��¼�
	HANDLE m_eventServerRead;			//����˶��¼�
	DWORD m_shareMemorySize;			//�����ڴ�Ĵ�С(��λ�ֽ�)
	bool m_isServer;					//�������Ƿ��Ƿ����

	std::queue<std::string> m_receiveDatas;
	CRITICAL_SECTION m_dataCs;

	std::queue<std::string> m_sendDatas;
	CRITICAL_SECTION m_sendCs;

	unsigned m_sendMaxSize;
	unsigned m_receiveMaxSize;

	bool m_threadWorking;				//�ж��߳��Ƿ���
	HANDLE m_hSendThread;				//����Ϣ�߳̾��
	HANDLE m_hReceiveThread;			//����Ϣ�߳̾��
};
