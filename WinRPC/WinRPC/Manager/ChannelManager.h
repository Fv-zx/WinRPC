#pragma once
#include <map>
#include <queue>
#include <string>
#include "../Channel/MemoryChannel.h"

class ChannelManager
{
public:
	ChannelManager();
	~ChannelManager();
	bool AddChannel(
		const std::string channelName,		//ͨ������
		bool isServer = false,				//�Ƿ��Ƿ����,Ĭ���ǿͻ���
		DWORD shareMemorySize = 1024 * 4,   //�����ڴ�Ĵ�С,Ĭ����4K
		unsigned sendMaxSize = 100,			//�������ݵ����洢����,Ĭ����100��
		unsigned receiveMaxSize = 100		//�������ݵ����洢����,Ĭ����100��
		);//���ͨ��

	bool IsChannelExist(const std::string channelName);//�ж�ͨ���Ƿ��Ѿ�����
	void AddChannelItem(const std::string channelName, MemoryChannel* pChannel);//��map�����ͨ��
	void DelChannelItem(const std::string channelName);//��mapɾ��ͨ��
	bool SendData(const std::string channelName, std::string data);//��ָ����ͨ����������
	bool GetData(const std::string channelName, std::queue<std::string>& data);//��ָ����ͨ����ȡ����(����)
private:
	std::map<std::string, MemoryChannel*> m_channelMaps;
	CRITICAL_SECTION m_channelMapsCS;
};
