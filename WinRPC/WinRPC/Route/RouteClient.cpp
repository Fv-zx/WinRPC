#include "RouteClient.h"

RouteClient::RouteClient(std::string routeManagerName, std::string clientName, unsigned channelMemSize, unsigned sendDatasMax, unsigned receiveDatasMax)
{
	m_routeManagerName = routeManagerName;
	m_clientName = clientName;
	m_channelMemSize = channelMemSize;
	m_sendDatasMax = sendDatasMax;
	m_receiveDatasMax = receiveDatasMax;
}

RouteClient::~RouteClient()
{

}

bool RouteClient::InitRouteClient()
{
	bool result = false;
	//Server��صĳ�ʼ��
	std::string routeServerNoticeEventName = "ROUTE_SERVER_NOTICE_" + m_routeManagerName + "_WINRPC";
	m_hServerNoticeEvent = CreateEventA(NULL, FALSE, FALSE, routeServerNoticeEventName.c_str());
	std::string routeServerMutexName = "ROUTE_SERVER_MUTEX_" + m_routeManagerName + "_WINRPC";
	m_hServerMutex = CreateMutexA(NULL, FALSE, routeServerMutexName.c_str());
	std::string routeServerMemName = "Global\\ROUTE_SERVER_SHARE_" + m_routeManagerName + "_WINRPC";
	m_serverTableMem = new ShareMemory(routeServerMemName);

	if (m_serverTableMem != NULL)
	{
		m_serverMemAddr = m_serverTableMem->OpenShareMem(NULL, 4096);
		if (m_serverMemAddr != NULL)
		{
			//��ȡ�����ڴ��еķ������б�
			std::string serverData;	//�����ڴ��д洢�ķ�������Ϣ
			serverData.resize(4096);
			if (WaitForSingleObject(m_hServerMutex, INFINITE) == WAIT_OBJECT_0)
			{
				m_serverTableMem->ReadShareMem(m_serverMemAddr, (void*)serverData.c_str(), 4096);
				ReleaseMutex(m_hServerMutex);//��ȡ�깲���ڴ��Ժ�,�ͷ���

				std::vector<ServerNode> serverNodes;
				ReadShareMemServerInfo(serverData, serverNodes);
				for (auto iter = serverNodes.begin(); iter != serverNodes.end(); iter++)
				{
					ServerNode serverNode = *iter;
					std::string serverName = serverNode.serverName;
					AddServer(serverName);
				}

				result = true;
			}
		}
	}
	return result;
}

bool RouteClient::AddServer(std::string serverName)
{
	bool result = false;
	const std::string channelName = serverName + "_" + m_clientName;
	//���ｨ��ͨ��ʹ����(����������+"_"+�ͻ�������),Ϊ���ܹ���������µķ������Ժ�,�ÿͻ����ܹ�����
	MemoryChannel* pChannel = new MemoryChannel(channelName, false , m_channelMemSize, m_sendDatasMax, m_receiveDatasMax);
	;
	if (pChannel != NULL)
	{
		if (pChannel->InitChannel() == NOT_ERROR)
		{
			m_serverChannelsMutex.lock();
			m_serverChannels[serverName] = pChannel;//��ӿͻ���ͨ��
			m_serverChannelsMutex.unlock();
			result = true;
		}
	}
	return result;
}

bool RouteClient::IsServerExist(std::string serverName)
{
	bool result = false;
	const std::string channelName = serverName + "_" + m_clientName;
	m_serverChannelsMutex.lock();
	if (m_serverChannels.find(channelName) != m_serverChannels.end())
	{
		result = true;
	}
	m_serverChannelsMutex.unlock();
	return result;
}

void RouteClient::BroadcastData(std::string data)
{
	m_serverChannelsMutex.lock();
	// �����еķ���������һ������(��������㲥����)
	for (auto iter = m_serverChannels.begin(); iter != m_serverChannels.end(); iter++)
	{
		MemoryChannel* pChannel = iter->second;
		pChannel->StoreSendData(data);
	}
	m_serverChannelsMutex.unlock();
}

void RouteClient::SendData(std::string serverName, std::string data)
{
	m_serverChannelsMutex.lock();
	//��ָ���ķ�������������(������)
	if (m_serverChannels.find(serverName) != m_serverChannels.end())
	{
		MemoryChannel* pChannel = m_serverChannels[serverName];
		if (pChannel != NULL)
		{
			pChannel->StoreSendData(data);
		}
	}
	m_serverChannelsMutex.unlock();
}

bool RouteClient::GetReceivedDatas(std::vector<MsgNode>& datas)
{
	bool result = false;
	return result;
}

unsigned __stdcall RouteClient::ServerInfoMonitorThread(LPVOID args)
{
	RouteClient* p = (RouteClient*)args;
	while (true)
	{
		if (WaitForSingleObject(p->m_hServerNoticeEvent, 5000) == WAIT_OBJECT_0)
		{
			std::map<std::string, ServerNode> serverNodes;
			if (WaitForSingleObject(p->m_hServerMutex, INFINITE) == WAIT_OBJECT_0)
			{
				std::string serverData;
				serverData.resize(4096);
				p->m_serverTableMem->ReadShareMem(p->m_serverMemAddr, (void*)serverData.c_str(), 4096);
				ReleaseMutex(p->m_hServerMutex);//��ʱ�ͷ���

				std::vector<ServerNode> serverNodes;
				ReadShareMemServerInfo(serverData, serverNodes);
				for (auto iter = serverNodes.begin(); iter != serverNodes.end(); iter++)
				{
					ServerNode serverNode = *iter;
					std::string serverName = serverNode.serverName;
					if (p->IsServerExist(serverName) == false) //�ж�,�����������Դ�Ѿ���������,���ٽ�����Դ����
					{
						p->AddServer(serverName);
					}
				}
			}
		}
	}
	return 0;
}