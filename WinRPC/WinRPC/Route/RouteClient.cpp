#include "RouteClient.h"

RouteClient::RouteClient(std::string routeManagerName, unsigned channelMemSize, unsigned sendDatasMax, unsigned receiveDatasMax)
{
	m_routeManagerName = routeManagerName;
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
	m_hServerMutex = CreateMutexA(NULL, TRUE, routeServerMutexName.c_str());
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

				if (serverData.empty() == false)//����洢��������Ϣ�Ĺ����ڴ���������,���ȶ�ȡԭ�е�����
				{
					Json::Value rootValue;
					Json::Reader reader;
					if (reader.parse(serverData, rootValue) == true)
					{
						if (rootValue["servers"].isArray())
						{
							for (int i = 0; i < rootValue["servers"].size(); i++)
							{
								Json::Value serverValue = rootValue["servers"][i];
								ServerNode serverNode;
								serverNode.serverName = serverValue["server_name"].asString();
								serverNode.pid = serverValue["pid"].asUInt();
								m_serverNodes[serverNode.serverName] = serverNode;
							}

							if (m_serverNodes.size() != 0)
							{
								for (auto iter = m_serverNodes.begin(); iter != m_serverNodes.end(); iter++)
								{
									std::string serverName = iter->first;
									AddServer(serverName);
								}
							}

							result = true;
						}
					}
				}
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

bool GetReceivedDatas(std::vector<MsgNode>& datas)
{
	bool result = false;
	return result;
}