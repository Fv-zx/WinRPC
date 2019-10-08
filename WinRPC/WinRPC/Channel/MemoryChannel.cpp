#include "MemoryChannel.h"
#include <process.h>

MemoryChannel::MemoryChannel(const std::string channelName, bool isServer, DWORD shareMemorySize, unsigned sendMaxSize, unsigned receiveMaxSize)
{
	m_channelName = channelName;
	m_shareMemorySize = shareMemorySize;
	m_isServer = isServer;
	m_sendMaxSize = sendMaxSize;
	m_receiveMaxSize = receiveMaxSize;
	InitializeCriticalSection(&m_dataCs);
	InitializeCriticalSection(&m_sendCs);
}

MemoryChannel::~MemoryChannel()
{
	DeleteCriticalSection(&m_dataCs);
	DeleteCriticalSection(&m_sendCs);
}

CHANNEL_ERROR MemoryChannel::InitChannel()
{
	CHANNEL_ERROR errorCode = CHANNEL_ERROR::NOT_ERROR;
	m_shareMemoryClient = new ShareMemory("SHARE_CF113892-23F7-4B4D-844D-5D4820BAEC97_" + m_channelName + "_client");
	m_shareMemoryServer = new ShareMemory("SHARE_CF113892-23F7-4B4D-844D-5D4820BAEC97_" + m_channelName + "_server");
	if (m_shareMemoryClient != NULL && m_shareMemoryServer != NULL)
	{
		m_shareMemoryClientAddr = m_shareMemoryClient->OpenShareMem(NULL, m_shareMemorySize, FILE_MAP_ALL_ACCESS);
		m_shareMemoryServerAddr = m_shareMemoryServer->OpenShareMem(NULL, m_shareMemorySize, FILE_MAP_ALL_ACCESS);
		if (m_shareMemoryClientAddr != NULL && m_shareMemoryServerAddr != NULL)
		{
			std::string clientReadEventName = "EVENT_CF113892-23F7-4B4D-844D-5D4820BAEC97_" + m_channelName + "_client";
			std::string serverReadEventName = "EVENT_CF113892-23F7-4B4D-844D-5D4820BAEC97_" + m_channelName + "_server";

			//�¼�������Ϊ�ֶ��ָ�
			m_eventClientRead = CreateEventA(NULL, TRUE, FALSE, clientReadEventName.c_str());
			m_eventServerRead = CreateEventA(NULL, TRUE, FALSE, serverReadEventName.c_str());

			if (m_eventClientRead != NULL && m_eventServerRead != NULL)
			{
				//������Ϣ�շ��߳�
				_beginthread(SendDataThread, 0, NULL);
				_beginthread(ReceiveDataThread, 0, NULL);
				errorCode = CHANNEL_ERROR::NOT_ERROR;
			}
			else
			{
				errorCode = CHANNEL_ERROR::EVENT_ERROR;
			}
		}
		else
		{
			errorCode = CHANNEL_ERROR::SHAREADDR_ERROR;
		}
	}
	else
	{
		errorCode = CHANNEL_ERROR::SHAREMEM_ERROR;
	}

	return errorCode;
}

void MemoryChannel::StoreSendData(std::string data)
{
	ubase::MyCriticalSection cs(&m_sendCs);
	if (m_sendDatas.size() < m_sendMaxSize)//������������ݳ���С����󳤶�,��ֱ�Ӳ���
	{
		m_sendDatas.push(data);
	}
	else//������г��ȴ��ڵ�����󳤶�,����̭��������һ������,���������µ�һ������
	{
		m_sendDatas.push(data);
		m_sendDatas.pop();
	}
}

bool MemoryChannel::GetSendData(std::string& data)
{
	ubase::MyCriticalSection cs(&m_sendCs);
	bool result = false;
	if (m_sendDatas.size() != 0)
	{
		data = m_sendDatas.front();
		m_sendDatas.pop();
		result = true;
	}
	return result;
}

void MemoryChannel::StoreReceiveData(std::string data)
{
	ubase::MyCriticalSection cs(&m_dataCs);
	if (m_receiveDatas.size() < m_receiveMaxSize)//������������ݳ���С����󳤶�,��ֱ�Ӳ���
	{
		m_receiveDatas.push(data);
	}
	else//������г��ȴ��ڵ�����󳤶�,����̭��������һ������,���������µ�һ������
	{
		m_receiveDatas.push(data);
		m_receiveDatas.pop();
	}
}

bool MemoryChannel::GetReceiveData(std::queue<std::string>& dataSet)
{
	ubase::MyCriticalSection cs(&m_dataCs);
	bool result = false;
	if (m_receiveDatas.size() != 0)
	{
		dataSet = m_receiveDatas;
		while (m_receiveDatas.empty() == false)
		{
			m_receiveDatas.pop();
		}
		result = true;
	}
	return result;
}

void MemoryChannel::SendDataThread(LPVOID args)
{
	MemoryChannel *p = (MemoryChannel*)args;
	HANDLE hEventRead = NULL;
	ShareMemory *pShareMem = NULL;
	void* pShareMemAddr = NULL;
	if (p->m_isServer == true) //�����ǰΪ��������,����ͻ��˹����ڴ淢������
	{
		hEventRead = p->m_eventClientRead;
		pShareMem = p->m_shareMemoryClient;
		pShareMemAddr = p->m_shareMemoryClientAddr;
	}
	else //�����ǰΪ�ͻ���,�������˹����ڴ淢������
	{
		hEventRead = p->m_eventServerRead;
		pShareMem = p->m_shareMemoryServer;
		pShareMemAddr = p->m_shareMemoryServerAddr;
	}

	while (true)
	{
		if (WaitForSingleObject(hEventRead, 0) == WAIT_TIMEOUT) //����ȴ��¼���ʱ,˵����ȡ�¼�û�б�����,˵�����Է�������
		{
			std::string sendData;
			if (p -> GetSendData(sendData) == true)
			{
				int writeSize = pShareMem->WriteShareMem(pShareMemAddr, (void*)sendData.c_str(), sendData.length());
				if (writeSize == sendData.length())
				{
					SetEvent(hEventRead);//֪ͨ���ݽ��ն˽��ж�ȡ����
				}
			}
		}
		else //�ȴ��¼��ɹ�,˵����ȡ�¼�������,˵�����ڹ����ڴ洦�ڷ�æ״̬,���ܹ�д��
		{
			Sleep(10);
		}
	}
}

void MemoryChannel::ReceiveDataThread(LPVOID args)
{
	MemoryChannel *p = (MemoryChannel*)args;
	HANDLE hEventRead = NULL;
	ShareMemory *pShareMem = NULL;
	void* pShareMemAddr = NULL;
	if (p->m_isServer == true) //�����ǰΪ��������,��ӷ���˹����ڴ��ȡ����
	{
		hEventRead = p->m_eventServerRead;
		pShareMem = p->m_shareMemoryServer;
		pShareMemAddr = p->m_shareMemoryServerAddr;
	}
	else //�����ǰΪ�ͻ���,��ӿͻ��˹����ڴ��ȡ����
	{
		hEventRead = p->m_eventClientRead;
		pShareMem = p->m_shareMemoryClient;
		pShareMemAddr = p->m_shareMemoryClientAddr;
	}

	while (true)
	{
		if (WaitForSingleObject(hEventRead, INFINITE) == WAIT_OBJECT_0)
		{
			std::string receiveData;
			receiveData.resize(p->m_shareMemorySize);
			pShareMem->ReadShareMem(pShareMemAddr, (void*)receiveData.c_str(), p->m_shareMemorySize);
			p->StoreReceiveData(receiveData);
			ResetEvent(hEventRead);//���ݴ洢�Ѿ����,֪ͨд��˿���д����
		}
	}
}