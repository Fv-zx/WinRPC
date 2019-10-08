#pragma once
#include <windows.h>
#include <string>
#include <iostream>

class ShareMemory
{
public:
	ShareMemory(const std::string shareMemName, bool createFile = false);
	~ShareMemory();

	void* OpenShareMem(void* addr, const unsigned length, DWORD protect);	//�򿪹����ڴ�
	int WriteShareMem(void* dest, void*src, unsigned size);					//д�����ڴ�
	int ReadShareMem(void* src, void*dest, unsigned size);					//�������ڴ�
private:
	bool CheckAddress(void* addr);	//����ڴ��ַ�Ƿ�Ϸ�
private:
	std::string m_shareMemName;		//�����ڴ������
	bool m_isCreateFile;			//�ļ������ڴ�or�ڴ�ҳ�����ڴ�
	void* m_shareMemAddress;		//�����ڴ�Ļ���ַ
	HANDLE m_semaphore;				//�ź�
	HANDLE m_fileMapping;			//�ļ����
	unsigned int m_shareMemSize;	//�����ڴ�Ĵ�С
};
