#include "RouteClient.h"

RouteClient::RouteClient(std::string routeManagerName, std::string clientName, unsigned channelMemSize, unsigned sendDatasMax, unsigned receiveDatasMax)
{
	m_routeManagerName = routeManagerName;
	m_clientName = clientName;
	m_channelMemSize = channelMemSize;
	m_sendDatasMax = sendDatasMax;
	m_receiveDatasMax = receiveDatasMax;
	m_serverInfoMonitorThRunning = true;
}

RouteClient::~RouteClient()
{
	if (m_hServerInfoMonitorThread != NULL)
	{
		m_serverInfoMonitorThRunning = false;
		WaitForSingleObject(m_hServerInfoMonitorThread, 5000);
		CloseHandle(m_hServerInfoMonitorThread);
	}

	if (m_hServerNoticeEvent != NULL)
	{
		CloseHandle(m_hServerNoticeEvent);
	}

	if (m_hServerMutex != NULL)
	{
		CloseHandle(m_hServerMutex);
	}

	if (m_serverTableMem != NULL)
	{
		delete m_serverTableMem;
	}

	//�������е�ͨ��
	for (auto iter = m_serverChannels.begin(); iter != m_serverChannels.end(); iter++)
	{
		MemoryChannel* p = iter->second;
		delete p;
	}
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
		m_hServerInfoMonitorThread = (HANDLE)_beginthreadex(NULL, 0, ServerInfoMonitorThread, (LPVOID)this, 0, NULL);
		if (m_serverMemAddr != NULL && m_hServerInfoMonitorThread != NULL)
		{
			//��ȡ�����ڴ��еķ������б�
			std::string serverData;	//�����ڴ��д洢�ķ�������Ϣ
			serverData.resize(4096);
			DWORD waitResult = WaitForSingleObject(m_hServerMutex, INFINITE);
			if (waitResult == WAIT_OBJECT_0 || waitResult == STATUS_ABANDONED_WAIT_0)
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
	m_serverChannelsMutex.lock();
	//�ж��Ƿ��Ѿ����ڸ÷�����,���û�и÷�����,����Ӹ÷�����
	if (IsServerExist(serverName) == false)
	{
		const std::string channelName = serverName + "_" + m_clientName;
		//���ｨ��ͨ��ʹ����(����������+"_"+�ͻ�������),Ϊ���ܹ���������µķ������Ժ�,�ÿͻ����ܹ�����
		MemoryChannel* pChannel = new MemoryChannel(channelName, false, m_channelMemSize, m_sendDatasMax, m_receiveDatasMax, RecvDataCallback, this);

		if (pChannel != NULL)
		{
			if (pChannel->InitChannel() == NOT_ERROR)
			{
				m_serverChannels[serverName] = pChannel;//��ӿͻ���ͨ��	
				result = true;
			}
		}
	}
	m_serverChannelsMutex.unlock();
	return result;
}

bool RouteClient::IsServerExist(std::string serverName)
{
	bool result = false;
	if (m_serverChannels.find(serverName) != m_serverChannels.end())
	{
		result = true;
	}
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

void RouteClient::GetReceivedDatas(std::vector<MsgNode>& datas)
{
	m_receiveDatasMutex.lock();
	while (m_receiveDatasQueue.empty() == false)
	{
		datas.push_back(m_receiveDatasQueue.front());
		m_receiveDatasQueue.pop();
	}
	m_receiveDatasMutex.unlock();
}

void RouteClient::StoreReceivedData(std::string serverName, std::string data)
{
	MsgNode msg;
	msg.clientOrServerName = serverName;
	msg.data = data;
	m_receiveDatasMutex.lock();
	if (m_receiveDatasQueue.size() >= m_receiveDatasMax)
	{
		m_receiveDatasQueue.pop();
		m_receiveDatasQueue.push(msg);
	}
	else
	{
		m_receiveDatasQueue.push(msg);
	}
	m_receiveDatasMutex.unlock();
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
					p->AddServer(serverName);
				}
			}
		}
	}
	return 0;
}

void __stdcall RouteClient::RecvDataCallback(const char* channelName, const char* data, unsigned int dataLength, void* pContext)
{
	RouteClient* p = (RouteClient*)pContext;
	if (p != NULL)
	{
		if (channelName != NULL && data != NULL)
		{
			std::string binData;
			binData.resize(dataLength);
			memcpy((char*)binData.c_str(), data, dataLength);

			std::vector<std::string> clientArray = StrSplit(channelName, "_");
			if (clientArray.size() == 2)
			{
				p->StoreReceivedData(clientArray[0], binData);
#if _DEBUG
				printf("%s:%s\n", clientArray[0].c_str(), binData.c_str());
#endif
			}
		}
	}
}