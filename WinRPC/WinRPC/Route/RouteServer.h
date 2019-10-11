#pragma once
#include "../Common/ShareMemory.h"
#include "../Channel/MemoryChannel.h"
#include <string>
#include <map>
#include <mutex>
#include <queue>
#include <vector>
#include <windows.h>

//����˵���ϸ��Ϣ
struct ClientNode
{
	std::string clientName;	//�ͻ��˵�����
	DWORD pid;				//�ͻ������ڽ��̵�PID
};

struct MsgNode
{
	std::string clientName;	//�ͻ��˵�����
	std::string data;		//���ݵ�����
};

class RouteServer
{
public:
	RouteServer(std::string routeManagerName, std::string serverName, unsigned channelMemSize = 1024 * 4, unsigned sendDatasMax = 100, unsigned receiveDatasMax = 100);
	~RouteServer();
	bool InitRouteManager();											//��ʼ��·�ɹ�����
	bool AddServerRoute(std::string serverName);						//��ӷ�����
	bool DelServer(std::string serverName);								//ɾ��������
	bool SendData(std::string clientName, std::string data);			//��ͻ��˷�������(���clientNameΪ��,�������еĿͻ��˷�������)
	bool GetSendData(MsgNode& data);									//�Ӷ�����,��ȡһ����Ҫ���͵�����
	bool StoreReceivedData(std::string clientName, std::string data);	//�洢�ӿͻ����յ�������
	bool GetReceivedData(std::vector<MsgNode>& data);					//��ȡ�ͻ��˷�������������
	bool AddClient(std::string clientName);								//���һ���µĿͻ���
	bool DelClient(std::string clientName);								//ɾ��һ���ͻ���
private:
	std::string m_routeManagerName;	//·�ɹ�����������
	std::string m_serverName;		//�˷���˵�����

	//Server���
	HANDLE m_hServerNoticeEvent;	//Server�����¼�
	ShareMemory* m_serverTableMem;	//���Server����Ϣ�Ĺ����ڴ�
	void* m_serverMemAddr;			//���Server��Ϣ�����ڴ���ڴ�ӳ���ַ
	HANDLE m_hServerMutex;			//Server·�ɹ����ڴ���ʻ�����(ȫ��)

	//�շ��������
	std::queue<MsgNode> m_receiveDatas;	//���մ�Client���͹������ݵĶ���
	std::mutex m_receiveDatasMutex;		//�������ݶ��в�����
	unsigned int m_receiveDatasMax;		//���ն��л�����󳤶�(�����������̭�����)
	std::queue<MsgNode> m_sendDatas;	//��Client�������ݵĶ���
	std::mutex m_sendDatasMutex;		//�������ݶ��в�����
	unsigned int m_sendDatasMax;		//���Ͷ��л�������󳤶�(�����������̭�����)

	//ͨ��ͨ�����
	std::map<std::string, MemoryChannel*> m_clientChannels;//�ͻ���Ƶ��
	std::mutex m_clientChannelsMutex;	//�ͻ���Ƶ����
	unsigned int m_channelMemSize;		//����ͨ�Źܵ��Ĺ����ڴ��С
};
