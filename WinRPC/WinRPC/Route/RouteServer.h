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
	RouteServer();
	~RouteServer();
	bool InitRouteManager();											//��ʼ��·�ɹ�����
	bool AddServerRoute(std::string serverName);						//��ӷ�����
	bool DelRoute(std::string serverName);								//ɾ��������
	bool SendData(std::string clientName, std::string data);			//��ͻ��˷�������(���clientNameΪ��,�������еĿͻ��˷�������)
	bool GetSendData(MsgNode& data);									//�Ӷ�����,��ȡһ����Ҫ���͵�����
	bool StoreReceivedData(std::string clientName, std::string data);	//�洢�ӿͻ����յ�������
	bool GetReceivedData(std::vector<MsgNode>& data);					//��ȡ�ͻ��˷�������������
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
	std::mutex m_receiveDatasMutex;
	std::queue<MsgNode> m_sendDatas;	//��Client�������ݵĶ���
	std::mutex m_sendDatasMutex;

	//ͨ��ͨ�����
	std::map<std::string, MemoryChannel*> m_clientChannels;//�ͻ���Ƶ��
};
