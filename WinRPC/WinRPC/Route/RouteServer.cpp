#include "RouteServer.h"
#include <json/json.h>

RouteServer::RouteServer()
{

}

RouteServer::~RouteServer()
{
	if (m_hRouteUpdateNoticeEvent != NULL)
	{
		CloseHandle(m_hRouteUpdateNoticeEvent);
	}

	if (m_hRouteMutex != NULL)
	{
		CloseHandle(m_hRouteMutex);
	}

	if (m_routeUpdateMem != NULL)
	{
		delete m_routeUpdateMem;
	}
}

bool RouteServer::InitRouteManager()
{
	bool result = false;
	std::string routeEventName = "ROUTE_SHARE_NOTICE_" + m_routeManagerName + "_WINRPC";
	m_hRouteUpdateNoticeEvent = CreateEventA(NULL, FALSE, FALSE, routeEventName.c_str());
	std::string routeMutexName = "ROUTE_SHARE_MUTEX_" + m_routeManagerName + "_WINRPC";
	m_hRouteMutex = CreateMutexA(NULL, TRUE, routeMutexName.c_str());
	std::string routeUpdateMemName = "Global\\ROUTE_SHARE_" + m_routeManagerName + "_WINRPC";
	m_routeUpdateMem = new ShareMemory(routeUpdateMemName);
	if (m_routeUpdateMem != NULL)
	{
		m_routeUpdateMemAddr = m_routeUpdateMem->OpenShareMem(NULL, 4096);
		if (m_routeUpdateMemAddr != NULL)
		{
			result = AddRoute(m_routeName);
			if (result == true)//�������·�ɱ�ɹ�,֪ͨ�����Ĺ���ģ��,Ҳ��ʱ����·�ɱ�
			{
				SetEvent(m_hRouteUpdateNoticeEvent);
			}
		}
	}
	return result;
}

/*
	�����ڴ��е�·����Ϣ�ṹ:
	{
		routes:[
			{"route_name":"hkxiaoyu118", "pid":2462},
			{"route_name":"zhoujielun", "pid":32452}
		]
	}
	route_name:·������
	pid:·�ɶ�Ӧ�Ľ���ID
*/
bool RouteServer::AddRoute(std::string routeName)
{
	bool result = false;
	if (WaitForSingleObject(m_hRouteMutex, INFINITE) == WAIT_OBJECT_0)
	{
		std::string oldRouteData;	//�ɵ�·����Ϣ
		std::string newRouteData;	//�µ�·����Ϣ
		oldRouteData.resize(4096);
		m_routeUpdateMem->ReadShareMem(m_routeUpdateMemAddr, (void*)oldRouteData.c_str(), 4096);
		if (oldRouteData.empty() == false)//����洢·����Ϣ�Ĺ����ڴ���������,���ȶ�ȡԭ�е�����
		{
			Json::Value rootValue;
			Json::Reader reader;
			if (reader.parse(oldRouteData, rootValue) == true)
			{
				if (rootValue["routes"].isArray())
				{
					Json::Value routeValue;
					Json::FastWriter writer;
					routeValue["route_name"] = routeName;
					routeValue["pid"] = (unsigned)GetCurrentProcessId();
					rootValue["routes"].append(routeValue);
					newRouteData = writer.write(rootValue);
				}
			}
		}
		else//����洢·����Ϣ�Ĺ����ڴ���û������,��ֱ����д����
		{
			Json::Value rootValue;
			Json::Value routeValue;
			Json::FastWriter writer;
			routeValue["route_name"] = routeName;
			routeValue["pid"] = (unsigned)GetCurrentProcessId();
			rootValue["routes"].append(routeValue);
			newRouteData = writer.write(rootValue);
		}

		if (newRouteData.empty() == false)
		{
			m_routeUpdateMem->WriteShareMem(m_routeUpdateMemAddr, (void*)newRouteData.c_str(), 4096);
			result = true;
		}
		
		ReleaseMutex(m_hRouteMutex);
	}
	return result;
}

bool RouteServer::DelRoute(std::string routeName)
{
	bool result = false;
	return result;
}

bool RouteServer::UpdateLocalRouteTable()
{
	bool result = false;
	if (WaitForSingleObject(m_hRouteMutex, INFINITE) == WAIT_OBJECT_0)
	{
		std::string routeData;
		routeData.resize(4096);
		m_routeUpdateMem->ReadShareMem(m_routeUpdateMemAddr, (void*)routeData.c_str(), 4096);
		if (routeData.empty() == false)
		{
			Json::Value rootValue;
			Json::Reader reader;
			if (reader.parse(routeData, rootValue) == true)
			{
				if (rootValue["routes"].isArray())
				{
					std::map<std::string, RouteNode> newRouteMap;
					for (int i = 0; i < rootValue["routes"].size(); i++)
					{
						RouteNode node;
						Json::Value routeValue = rootValue["routes"][i];
						node.routeName = routeValue["route_name"].asString();
						node.pid = routeValue["pid"].asUInt();
						newRouteMap[node.routeName] = node;
					}

					//ֱ���滻��ԭ�е�routemap
					m_routeMapsMutex.lock();
					m_routeMaps = newRouteMap;
					m_routeMapsMutex.unlock();

					result = true;
				}
			}
		}
	}
	return result;
}

unsigned __stdcall RouteServer::MonitorRouteTableUpdateThread(LPVOID args)
{
	RouteServer* p = (RouteServer*)args;

	while (true)
	{
		if (WaitForSingleObject(p->m_hRouteUpdateNoticeEvent, INFINITE) == WAIT_OBJECT_0)//�յ�����·�ɱ��֪ͨ
		{
			//����·�ɱ�
			p->UpdateLocalRouteTable();
		}
	}
	return 0;
}