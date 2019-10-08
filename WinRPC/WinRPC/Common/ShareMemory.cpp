#include "ShareMemory.h"


ShareMemory::ShareMemory(const std::string shareMemName, bool createFile)
{
	m_isCreateFile = createFile;
	m_shareMemName = shareMemName;
	m_semaphore = CreateSemaphoreA(NULL, 1, 1, NULL);
	m_fileMapping = INVALID_HANDLE_VALUE;
	m_shareMemAddress = NULL;
}


ShareMemory::~ShareMemory()
{
	if (m_shareMemAddress != NULL)
	{
		UnmapViewOfFile(m_shareMemAddress);
	}
	
	if (m_semaphore != NULL)
	{
		CloseHandle(m_semaphore);
	}
	
	if (m_fileMapping != NULL)
	{
		CloseHandle(m_fileMapping);
	}
}

/*
	����ô����ļ�ӳ��,�����ڴ治����ִ洢��release�����Υ����ʵ�����,���ǻ��ڴ����Ͻ���һ���ļ�
	�ļ��������ɲ���shareMemName����.�����ҳ�ļ�ӳ��,�򲻻��ڴ����Ͻ���һ���ļ�
	Ĭ��ʹ���ڴ�ҳ�ļ�ӳ��
*/
void* ShareMemory::OpenShareMem(void* addr, const unsigned length)
{
	m_shareMemSize = length;
	HANDLE fileHandle = INVALID_HANDLE_VALUE;
	//���ʹ�ô����ļ�ӳ��
	if (m_isCreateFile == true)
	{
		fileHandle = CreateFileA(
			m_shareMemName.c_str(), 
			GENERIC_READ | GENERIC_WRITE, 
			0,
			NULL, 
			OPEN_ALWAYS, 
			FILE_ATTRIBUTE_NORMAL, 
			0
		);
	}

	do 
	{
		//���ȳ��Դ򿪶�Ӧ���ƵĹ����ڴ�
		//�����ʧ����˵��û�и����ƵĹ����ڴ�,Ȼ�����´���һ��
		m_fileMapping = OpenFileMappingA(FILE_MAP_WRITE | FILE_MAP_READ, false, m_shareMemName.c_str());
		if (m_fileMapping != NULL)
		{
			break;
		}
		m_fileMapping = CreateFileMappingA(fileHandle, NULL, PAGE_READWRITE, 0, length, m_shareMemName.c_str());
		if (m_fileMapping == NULL)
		{
			return NULL;
		}
	} while (0);
	m_shareMemAddress = MapViewOfFileEx(m_fileMapping, FILE_MAP_ALL_ACCESS, 0, 0, length, addr);
	CloseHandle(fileHandle);
	return m_shareMemAddress;
}

int ShareMemory::WriteShareMem(void* dest, void*src, unsigned size)
{
	if (!CheckAddress(dest))
		return -1;
	int writeCount = (int)m_shareMemAddress + m_shareMemSize - (int)dest;
	if (writeCount > size)
		writeCount = size;
	//����semaphore���б���ӳ�������ͬһ���̵Ĳ�ͬ�̵߳���ʱ��Ž��б���,��ֹ����߳�ͬʱд�룩
	WaitForSingleObject(m_semaphore, INFINITE);
	memset(dest, 0, m_shareMemSize);//����Ҫ������ڴ�
	memcpy(dest, src, writeCount);
	ReleaseSemaphore(m_semaphore, 1, NULL);
	FlushViewOfFile(m_shareMemAddress, writeCount);
	return writeCount;
}

int ShareMemory::ReadShareMem(void* src, void*dest, unsigned size)
{
	if (!CheckAddress(src))
		return -1;
	int readCount = (int)m_shareMemAddress + m_shareMemSize - (int)src;
	if (readCount > size)
		readCount = size;
	memcpy(dest, src, readCount);
	return readCount;
}

bool ShareMemory::CheckAddress(void* addr)
{
	if (((int)addr < (int)m_shareMemAddress) || ((int)addr > (int)m_shareMemAddress + m_shareMemSize))
	{
		return false;
	}
	return true;
}