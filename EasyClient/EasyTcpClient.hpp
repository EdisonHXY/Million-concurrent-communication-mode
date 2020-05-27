#ifndef _EasyTcpClient_hpp_
#define _EasyTcpClient_hpp_

#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
	#define _WINSOCK_DEPRECATED_NO_WARNINGS //for inet_pton()
	#define _CRT_SECURE_NO_WARNINGS
	#include <windows.h>
	#include <WinSock2.h>
	#pragma comment(lib, "ws2_32.lib")
#else
	#include <unistd.h>
	#include <sys/socket.h>
	#include <sys/types.h>
	#include <arpa/inet.h>
	#include <netinet/in.h>
	#include <sys/select.h>
	//��Unix��û����Щ�꣬Ϊ�˼��ݣ��Լ�����
	#define SOCKET int
	#define INVALID_SOCKET  (SOCKET)(~0)
	#define SOCKET_ERROR            (-1)
#endif

//���ջ������Ĵ�С
#ifndef RECV_BUFF_SIZE
#define RECV_BUFF_SIZE 10240*5
#endif // !RECV_BUFF_SIZE


#include <iostream>
#include <string.h>
#include <stdio.h>
#include "MessageHeader.hpp"

using namespace std;

class EasyTcpClient
{
public:
	EasyTcpClient() :_sock(INVALID_SOCKET), _isConnect(false), _lastPos(0) {
		//memset(_recvBuff, 0, sizeof(_recvBuff));
		memset(_recvMsgBuff, 0, sizeof(_recvMsgBuff));
	}
	virtual ~EasyTcpClient() { CloseSocket(); }
public:
	void InitSocket();  //��ʼ��socket
	void CloseSocket(); //�ر�socket
	bool Onrun();       //����������Ϣ
	bool isRun() { return ((_sock != INVALID_SOCKET) && _isConnect); }       //�жϵ�ǰ�ͻ����Ƿ�������
	int  ConnectServer(const char* ip, unsigned int port); //���ӷ�����
	//ʹ��RecvData�����κ����͵����ݣ�Ȼ����Ϣ��ͷ���ֶδ��ݸ�OnNetMessage()�����У�������Ӧ��ͬ���͵���Ϣ
	int  RecvData();                                //��������
	virtual void OnNetMessage(DataHeader* header);  //��Ӧ������Ϣ
	int  SendData(DataHeader* header, int nLen)   ; //��������
private:
	SOCKET _sock;
	bool   _isConnect;                       //��ǰ�Ƿ�����
	char   _recvMsgBuff[RECV_BUFF_SIZE];     //��Ϣ���ջ�����
	int    _lastPos;                         //��Ϣ���ջ��������ݵĽ�βλ��
};

void EasyTcpClient::InitSocket()
{
	//���֮ǰ�������ˣ��رվ����ӣ�����������
	if (isRun())
	{
		std::cout << "<Socket=" << (int)_sock << ">���رվ����ӣ�������������" << std::endl;
		CloseSocket();
	}

#ifdef _WIN32
	WORD ver = MAKEWORD(2, 2);
	WSADATA dat;
	WSAStartup(ver, &dat);
#endif

	_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (INVALID_SOCKET == _sock) {
		std::cout << "ERROR:����socketʧ��!" << std::endl;
	}
	else {
		//std::cout << "<Socket=" << (int)_sock << ">������socket�ɹ�!" << std::endl;
	}
}

int EasyTcpClient::ConnectServer(const char* ip, unsigned int port)
{
	if (!isRun())
	{
		InitSocket();
	}

	//����Ҫ���ӵķ���˵�ַ��ע�⣬��ͬƽ̨�ķ����IP��ַҲ��ͬ��
	struct sockaddr_in _sin = {};
#ifdef _WIN32
	_sin.sin_addr.S_un.S_addr = inet_addr(ip);
#else
	_sin.sin_addr.s_addr = inet_addr(ip);
#endif
	_sin.sin_family = AF_INET;
	_sin.sin_port = htons(port);

	//���ӷ����
	int ret = connect(_sock, (struct sockaddr*)&_sin, sizeof(_sin));
	if (SOCKET_ERROR == ret) {
		std::cout << "<Socket=" << (int)_sock << ">�����ӷ����(" << ip << "," << port << ")ʧ��!" << std::endl;
	}
	else {
		_isConnect = true;
		//std::cout << "<Socket=" << (int)_sock << ">�����ӷ����(" << ip << "," << port << ")�ɹ�!" << std::endl;
	}
	
	return ret;
}

void EasyTcpClient::CloseSocket()
{
	if (_sock != INVALID_SOCKET)
	{
#ifdef _WIN32
		closesocket(_sock);
		WSACleanup();
#else
		close(_sock);
#endif
		_sock = INVALID_SOCKET;
		_isConnect = false;
	}
}

bool EasyTcpClient::Onrun()
{
	if (isRun())
	{
		fd_set fdRead;
		FD_ZERO(&fdRead);
		FD_SET(_sock, &fdRead);

		struct timeval t = { 0,0 };
		int ret = select(_sock + 1, &fdRead, NULL, NULL, &t);
		if (ret < 0)
		{
			std::cout << "<Socket=" << _sock << ">��select����" << std::endl;
			return false;
		}
		if (FD_ISSET(_sock, &fdRead)) //�������������ݷ��͹�����������ʾ����
		{
			FD_CLR(_sock, &fdRead);
			if (-1 == RecvData())
			{
				std::cout << "<Socket=" << _sock << ">�����ݽ���ʧ�ܣ��������ѶϿ���" << std::endl;
				CloseSocket();
				return false;
			}
		}
		return true;
	}
	return false;
}

int EasyTcpClient::RecvData()
{
	char *_recvBuff = _recvMsgBuff + _lastPos;
	int _nLen = recv(_sock, _recvBuff, RECV_BUFF_SIZE- _lastPos, 0);
	if (_nLen < 0) {
		std::cout << "<Socket=" << _sock << ">��recv��������" << std::endl;
		return -1;
	}
	else if (_nLen == 0) {
		std::cout << "<Socket=" << _sock << ">����������ʧ�ܣ�������ѹر�!" << std::endl;
		return -1;
	}
	//std::cout << "_nLen=" << _nLen << std::endl;
	
	//����һ������Ҫ��)����ȡ�����ݿ�������Ϣ������
	//memcpy(_recvMsgBuff + _lastPos, _recvBuff, _nLen);

	_lastPos += _nLen;
	//���_recvMsgBuff�е����ݳ��ȴ��ڵ���DataHeader
	while (_lastPos >= sizeof(DataHeader))
	{
		DataHeader* header = (DataHeader*)_recvMsgBuff;
		//���_lastPos��λ�ô��ڵ���һ�����ݰ��ĳ��ȣ���ô�ͻ�������ݰ����д���
		if (_lastPos >= header->dataLength)
		{
			//ʣ��δ������Ϣ�������ĳ���
			int nSize = _lastPos - header->dataLength;
			//����������Ϣ
			OnNetMessage(header);
			//�������֮�󣬽�_recvMsgBuff��ʣ��δ�����ֵ�����ǰ��
			memcpy(_recvMsgBuff, _recvMsgBuff + header->dataLength, nSize);
			_lastPos = nSize;
		}
		else {
			//��Ϣ������ʣ�����ݲ���һ��������Ϣ
			break;
		}
	}
	return 0;
}

void EasyTcpClient::OnNetMessage(DataHeader* header)
{
	switch (header->cmd)
	{
	case CMD_LOGIN_RESULT:   //������ص��ǵ�¼�Ľ��
	{
		LoginResult* loginResult = (LoginResult*)header;
		//std::cout << "<Socket=" << _sock << ">���յ���������ݣ�CMD_LOGIN_RESULT,���ݳ��ȣ�" << loginResult->dataLength << "�����Ϊ��" << loginResult->result << std::endl;
	}
	break;
	case CMD_LOGOUT_RESULT:  //������˳��Ľ��
	{
		LogoutResult* logoutResult = (LogoutResult*)header;
		//std::cout << "<Socket=" << _sock << ">���յ���������ݣ�CMD_LOGOUT_RESULT,���ݳ��ȣ�" << logoutResult->dataLength << "�����Ϊ��" << logoutResult->result << std::endl;
	}
	break;
	case CMD_NEW_USER_JOIN:  //�����û�����
	{
		NewUserJoin* newUserJoin = (NewUserJoin*)header;
		//std::cout << "<Socket=" << _sock << ">���յ���������ݣ�CMD_NEW_USER_JOIN,���ݳ��ȣ�" << newUserJoin->dataLength << "�����û�SocketΪ��" << newUserJoin->sock << std::endl;
	}
	break;
	case CMD_ERROR:  //������Ϣ
	{
		//������Ϣ�����;���DataHeader�ģ����ֱ��ʹ��header����
		//std::cout << "<Socket=" << _sock << ">���յ���������ݣ�CMD_ERROR�����ݳ��ȣ�" << header->dataLength << std::endl;
	}
	break;
	default:
	{
		//std::cout << "<Socket=" << _sock << ">���յ���������ݣ�δ֪���͵���Ϣ�����ݳ��ȣ�" << header->dataLength << std::endl;
	}
	}
}

int EasyTcpClient::SendData(DataHeader* header,int nLen) 
{
	int ret = SOCKET_ERROR;
	if (isRun() && header)
	{
		ret = send(_sock, (const char*)header, nLen, 0);
		if (ret == SOCKET_ERROR) {
			CloseSocket();
			printf("Client:socket<%d>�������ݴ��󣬹رտͻ�������\n", static_cast<int>(_sock));
		}
	}
	return ret;
}

#endif // !_EasyTcpClient_hpp_
