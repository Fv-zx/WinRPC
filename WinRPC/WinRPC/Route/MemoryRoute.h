#pragma once
#include <string>
#include <map>
#include <queue>
#include "../Common/ShareMemory.h"
#include "../Common/MyCriticalSection.h"


class MemoryRoute
{
public:
	MemoryRoute();
	~MemoryRoute();

	bool InitRoute(); //·�ɳ�ʼ��
	static unsigned __stdcall SendDataThread(LPVOID args);		//���������߳�
	static unsigned __stdcall ReceiveDataThread(LPVOID args);	//���������߳�
private:
	std::map<std::string, ShareMemory*> m_topicMaps;
	std::map<std::string, HANDLE> m_eventHandleMaps;
	HANDLE m_hSendDataThread;
	HANDLE m_hReceiveDataThread;
};
