#pragma once
#include "../Common/Utility.h"
#include "../TinyXML2/tinyxml2.h"
#include <queue>
#include <string>
#include <windows.h>

bool ReadShareMemServerInfo(std::string shareMemStr, std::vector<ServerNode>& serverNodes); //��ȡ�����ڴ��еķ�������Ϣ
std::string CreateShareMemServerInfo(std::vector<ServerNode> serverNodes);					//���ù����ڴ��еķ�������Ϣ