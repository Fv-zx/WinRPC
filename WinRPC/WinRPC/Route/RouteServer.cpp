#include "RouteServer.h"

RouteServer::RouteServer(std::string routeManagerName, std::string serverName, unsigned channelMemSize, unsigned sendDatasMax, unsigned receiveDatasMax)
{
	m_routeManagerName = routeManagerName;
	m_serverName = serverName;
	m_channelMemSize = channelMemSize;
	m_sendDatasMax = sendDatasMax;
	m_receiveDatasMax = receiveDatasMax;
	m_sendDataThreadRunning = true;
}

RouteServer::~RouteServer()
{
	if (m_hSendDataThread != NULL)
	{
		m_sendDataThreadRunning = false;//�����߳�
		WaitForSingleObject(m_hSendDataThread, 5000);
		CloseHandle(m_hSendDataThread);
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
}

bool RouteServer::InitRouteManager()
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
		m_hSendDataThread = (HANDLE)_beginthreadex(NULL, 0, SendDataThread, (LPVOID)this, 0, NULL);
		if (m_serverMemAddr != NULL && m_hSendDataThread != NULL)
		{
			result = AddServerRoute(m_serverName);
			if (result == true)//������·�������ɹ�,֪ͨ���пͻ���,Ҳ��ʱ���·�������
			{
				SetEvent(m_hServerNoticeEvent);
			}
		}
	}
	return result;
}

bool RouteServer::AddServerRoute(std::string serverName)
{
	bool result = false;
	DWORD waitResult = WaitForSingleObject(m_hServerMutex, INFINITE);
	if (waitResult == WAIT_OBJECT_0 || waitResult == STATUS_ABANDONED_WAIT_0)
	{
		std::string oldRouteData;	//�ɵķ�������Ϣ
		std::string newRouteData;	//�µķ�������Ϣ
		oldRouteData.resize(4096);
		m_serverTableMem->ReadShareMem(m_serverMemAddr, (void*)oldRouteData.c_str(), 4096);
		
		//�����µķ���������
		std::vector<ServerNode> serverNodes;
		ReadShareMemServerInfo(oldRouteData, serverNodes);
		ServerNode serverNode;
		serverNode.serverName = serverName;
		serverNode.pid = (unsigned)GetCurrentProcessId();
		serverNodes.push_back(serverNode);
		newRouteData = CreateShareMemServerInfo(serverNodes);

		if (newRouteData.empty() == false)
		{
			m_serverTableMem->WriteShareMem(m_serverMemAddr, (void*)newRouteData.c_str(), 4096);
			result = true;
		}
		
		ReleaseMutex(m_hServerMutex);
	}
	return result;
}

//������������Ϣ�ӹ����ڴ���ɾ��,��֪ͨ���еĿͻ��˸��·������б�
bool RouteServer::DelServer(std::string serverName)
{
	bool result = false;
	if (WaitForSingleObject(m_hServerMutex, INFINITE) == WAIT_OBJECT_0)
	{
		std::string oldRouteData;	//�ɵķ�������Ϣ
		std::string newRouteData;	//�µķ�������Ϣ
		oldRouteData.resize(4096);
		m_serverTableMem->ReadShareMem(m_serverMemAddr, (void*)oldRouteData.c_str(), 4096);

		//�����µķ���������(ɾ������������)
		std::vector<ServerNode> serverNodes;
		ReadShareMemServerInfo(oldRouteData, serverNodes);
		//ɾ����ǰ������
		for (auto iter = serverNodes.begin(); iter != serverNodes.end(); iter++)
		{
			ServerNode serverNode = *iter;
			if (serverNode.serverName == serverName)
			{
				serverNodes.erase(iter);
				break;
			}
		}
		newRouteData = CreateShareMemServerInfo(serverNodes);

		if (newRouteData.empty() == false)
		{
			m_serverTableMem->WriteShareMem(m_serverMemAddr, (void*)newRouteData.c_str(), 4096);
			result = true;
		}

		ReleaseMutex(m_hServerMutex);

		if (result == true)
		{
			SetEvent(m_hServerNoticeEvent);
		}
	}
	return result;
}

bool RouteServer::SendData(std::string clientName, std::string data)
{
	m_sendDatasMutex.lock();
	MsgNode msg;
	msg.clientOrServerName = clientName;
	msg.data = data;
	m_sendDatas.push(msg);
	m_sendDatasMutex.unlock();
	return true;
}

bool RouteServer::GetSendData(std::queue<MsgNode>& data)
{
	bool result = false;
	m_sendDatasMutex.lock();
	if (m_sendDatas.size() != 0)
	{
		//�����Ͷ������ݸ��Ƹ�������
		data = m_sendDatas;
		//��շ��Ͷ���
		while (m_sendDatas.empty() == false)
		{
			m_sendDatas.pop();
		}
		result = true;
	}
	m_sendDatasMutex.unlock();
	return result;
}

void RouteServer::StoreReceivedData(std::string clientName, std::string data)
{
	MsgNode msg;
	msg.clientOrServerName = clientName;
	msg.data = data;
	m_receiveDatasMutex.lock();
	if (m_receiveDatas.size() >= m_receiveDatasMax)
	{
		m_receiveDatas.pop();
		m_receiveDatas.push(msg);
	}
	else 
	{
		m_receiveDatas.push(msg);
	}
	m_receiveDatasMutex.unlock();
}

void RouteServer::GetReceivedData(std::vector<MsgNode>& data)
{
	bool result = false;
	m_receiveDatasMutex.lock();
	while (m_receiveDatas.empty() == false)
	{
		data.push_back(m_receiveDatas.front());
		m_receiveDatas.pop();
	}
	m_receiveDatasMutex.unlock();
}

bool RouteServer::AddClient(std::string clientName)
{
	bool result = false;
	const std::string channelName = m_serverName + "_" + clientName;
	//���ｨ��ͨ��ʹ����(����������+"_"+�ͻ�������),Ϊ���ܹ���������µķ������Ժ�,�ÿͻ����ܹ�����
	//�Իص������ķ�ʽ���н��տͻ��˷���������
	MemoryChannel* pChannel = new MemoryChannel(channelName, true, m_channelMemSize, m_sendDatasMax, m_receiveDatasMax, RecvDataCallback, this);
	if (pChannel != NULL)
	{
		if (pChannel->InitChannel() == NOT_ERROR)
		{
			m_clientChannelsMutex.lock();
			m_clientChannels[clientName] = pChannel;//��ӿͻ���ͨ��
			m_clientChannelsMutex.unlock();
			result = true;
		}
	}
	return result;
}

bool RouteServer::DelClient(std::string clientName)
{
	bool result = false;
	m_clientChannelsMutex.lock();
	if (m_clientChannels.find(clientName) != m_clientChannels.end())
	{
		MemoryChannel* pChannel = m_clientChannels[clientName];
		delete pChannel;
		m_clientChannels.erase(clientName);//ɾ��ָ�����ƵĿͻ���
		result = true;
	}
	m_clientChannelsMutex.unlock();
	return result;
}

void RouteServer::SendDataToChannel(const MsgNode& msgNode)
{
	m_clientChannelsMutex.lock();
	if (m_clientChannels.find(msgNode.clientOrServerName) != m_clientChannels.end())
	{
		MemoryChannel* p = m_clientChannels[msgNode.clientOrServerName];
		p->StoreSendData(msgNode.data);
	}
	m_clientChannelsMutex.unlock();
}

void RouteServer::RecvDataCallback(const char* channelName, const char* data, unsigned int dataLength, void* pContext)
{
	RouteServer* p = (RouteServer*)pContext;
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
				p->StoreReceivedData(clientArray[1], binData);
#if _DEBUG
				printf("%s:%s\n", clientArray[1].c_str(), binData.c_str());
#endif
			}
		}
	}
}

unsigned __stdcall RouteServer::SendDataThread(LPVOID args)
{
	RouteServer* p = (RouteServer*)args;

	while (true)
	{
		std::queue<MsgNode> msgNodeQueue;
		if (p->GetSendData(msgNodeQueue) == true)
		{
			while (msgNodeQueue.empty() == false)
			{
				MsgNode msgNode = msgNodeQueue.front();
				msgNodeQueue.pop();
				p->SendDataToChannel(msgNode);
			}
		}

		//���ֽ����ź�,�����߳�
		if (p->m_sendDataThreadRunning == false)
		{
			break;
		}
		Sleep(50);
	}
	return 0;
}