#pragma once
#include "../Common/ShareMemory.h"
#include "MemoryRoute.h"
#include <string>
#include <map>
#include <mutex>
#include <windows.h>

//·��������ϸ��Ϣ
struct RouteNode
{
	std::string routeName;	//·����������
	DWORD pid;				//·�������ڽ��̵�PID
};

class RouteServer
{
public:
	RouteServer();
	~RouteServer();
	bool InitRouteManager();				//��ʼ��·�ɹ�����
	bool AddRoute(std::string routeName);	//��·�ɹ�������,���·����
	bool DelRoute(std::string routeName);	//��·�ɹ�������,ɾ��·����
	bool UpdateLocalRouteTable();			//���±��ر����·�ɱ�
	static unsigned __stdcall MonitorRouteTableUpdateThread(LPVOID args);//����·�ɱ���Ϣ���߳�
private:
	std::string m_routeManagerName;		//·�ɹ���������
	HANDLE m_hRouteUpdateNoticeEvent;	//·�ɸ����¼�
	ShareMemory* m_routeUpdateMem;		//���·����Ϣ�Ĺ����ڴ�
	void* m_routeUpdateMemAddr;			//���·����Ϣ�����ڴ���ڴ�ӳ���ַ
	HANDLE m_hRouteMutex;				//·�ɹ����ڴ���ʻ�����(ȫ��)
	MemoryRoute* m_memRoute;			//�ڴ�·��
	std::string m_routeName;			//��·����������

	std::map<std::string, RouteNode> m_routeMaps;	//·�ɱ������
	std::mutex m_routeMapsMutex;					//·�ɱ����ݿ�����
};
