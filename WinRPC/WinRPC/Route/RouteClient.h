#pragma once
#include <string>
#include <queue>
#include <vector>
#include <map>
#include <mutex>
#include <process.h>
#include "../Common/Utility.h"
#include "../Common/ShareMemory.h"
#include "../Channel/MemoryChannel.h"
#include "RouteUtility.h"

class RouteClient
{
public:
	RouteClient(std::string routeManagerName, std::string clientName, unsigned channelMemSize = 1024 * 4, unsigned sendDatasMax = 100, unsigned receiveDatasMax = 100);
	~RouteClient();
	bool InitRouteClient();
	bool AddServer(std::string serverName);	//��ӷ����
	bool IsServerExist(std::string serverName);//�жϷ�����Ƿ��Ѿ�����
	void BroadcastData(std::string data);	//���������������(�㲥��ʽ)
	void SendData(std::string serverName, std::string data);//���������������(������)
	void GetReceivedDatas(std::vector<MsgNode>& datas);//��ȡ���յ�����(����)
	void StoreReceivedData(std::string serverName, std::string data);//�洢�ӷ������յ�������
	static unsigned __stdcall ServerInfoMonitorThread(LPVOID args);//���ӷ�������Ϣ���߳�
	static void __stdcall RecvDataCallback(const char* channelName, const char* data, unsigned int dataLength, void* pContext);//�������ݵĻص�����
private:
	std::string m_routeManagerName;	//·�ɹ�����������
	std::string m_clientName;		//���ͻ��˵�����
	unsigned int m_channelMemSize;	//ͨ��ʹ�õĹ����ڴ��С
	unsigned int m_sendDatasMax;	//���Ͷ��е���󳤶�
	unsigned int m_receiveDatasMax;	//���ն��е���󳤶�

	std::queue<MsgNode> m_receiveDatasQueue;	//������Ϣ����
	std::mutex m_receiveDatasMutex;				//������Ϣ������

	HANDLE m_hServerNoticeEvent;
	HANDLE m_hServerMutex;
	ShareMemory* m_serverTableMem;
	void* m_serverMemAddr;

	std::map<std::string, MemoryChannel*> m_serverChannels;	//������ͨ��
	std::mutex m_serverChannelsMutex;						//������ͨ����
	HANDLE m_hServerInfoMonitorThread;						//��������Ϣ�����߳̾��
	bool m_serverInfoMonitorThRunning;						//��������Ϣ�����߳��Ƿ�����
};
