#ifndef _EasyTcpClient_hpp_
#define _EasyTcpClient_hpp_

#ifdef _WIN32
#define FD_SETSIZE 10240
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


#ifndef RECV_BUFF_SIZE
#define RECV_BUFF_SIZE 10240*5        //���ջ������Ĵ�С
#endif // !RECV_BUFF_SIZE

#ifndef SEND_BUFF_SIZE
#define SEND_BUFF_SIZE RECV_BUFF_SIZE //���ͻ������Ĵ�С
#endif // !SEND_BUFF_SIZE

#include <iostream>
#include <string.h>
#include <stdio.h>
#include <vector>
#include <mutex>
#include <atomic>
#include <functional> 
#include <map>
#include <memory>
#include "MessageHeader.hpp"
#include "CELLTimestamp.hpp"
#include "CELLTask.hpp"
//using namespace std;

class CellServer;

//�ͻ����������ͣ�������װһ���ͻ���
class ClientSocket
{
public:
	ClientSocket(SOCKET sockfd = INVALID_SOCKET) :_sock(sockfd), _lastRecvPos(0), _lastSendPos(0) {
		memset(_recvMsgBuff, 0, sizeof(_recvMsgBuff));
		memset(_sendMsgBuff, 0, sizeof(_sendMsgBuff));
	}

	SOCKET sockfd() { return _sock; }
	char   *recvMsgBuff() { return _recvMsgBuff; }
	int    getRecvLastPos() { return _lastRecvPos; }
	void   setRecvLastPos(int pos) { _lastRecvPos = pos; }

	char   *sendMsgBuff() { return _sendMsgBuff; }
	int    getSendLastPos() { return _lastSendPos; }
	void   setSendLastPos(int pos) { _lastSendPos = pos; }

	int    SendData(std::shared_ptr<DataHeader>& header);
private:
	SOCKET _sock;                         //�ͻ���socket

	char   _recvMsgBuff[RECV_BUFF_SIZE];  //��Ϣ���ջ�����
	int    _lastRecvPos;                  //��Ϣ���ջ�����������β��λ��

	char   _sendMsgBuff[SEND_BUFF_SIZE];  //��Ϣ���ͻ�����
	int    _lastSendPos;                  //��Ϣ���ͻ�����������β��λ��
};

//�����¼��ӿ�
class INetEvent {
public:
	virtual void OnClientJoin(std::shared_ptr<ClientSocket>& pClient) = 0;     //�ͻ��˼����¼�
	virtual void OnClientLeave(std::shared_ptr<ClientSocket>& pClient) = 0;    //�ͻ����뿪�¼�
	virtual void OnNetMsg(CellServer* pCellServer, std::shared_ptr<ClientSocket>& pClient, DataHeader* header) = 0; //���յ��ͻ�����Ϣ�¼�
	virtual void OnNetRecv(std::shared_ptr<ClientSocket>& pClient) = 0;        //recv����ִ���¼�
};

//������Ϣ������
class CellSendMsg2ClientTask :public CellTask
{
public:
	CellSendMsg2ClientTask(std::shared_ptr<ClientSocket>& pClient, std::shared_ptr<DataHeader>& header)
		:_pClient(pClient), _pHeader(header) {}

	void doTask()
	{
		_pClient->SendData(_pHeader);
	}
private:
	std::shared_ptr<ClientSocket> _pClient; //���͸��ĸ��ͻ���
	std::shared_ptr<DataHeader>   _pHeader; //Ҫ���͵����ݵ�ͷָ��
};

//����ͻ��Ĵӷ�������
class CellServer
{
public:
	CellServer(SOCKET sock = INVALID_SOCKET) :_sock(sock), maxSock(_sock), _pthread(nullptr), _pNetEvent(nullptr), _clients_change(false) {
		memset(&_fdRead_bak, 0, sizeof(_fdRead_bak));
	}
	~CellServer() { CloseSocket(); }
public:
	bool   isRun() { return _sock != INVALID_SOCKET; }
	void   CloseSocket();
public:
	size_t getClientCount() { return _clients.size() + _clientsBuff.size(); } //���ص�ǰ�ͻ��˵�����
	void   setEventObj(INetEvent* event) { _pNetEvent = event; }              //�����¼����󣬴˴��󶨵���EasyTcpServer
public:
	bool   Onrun();
	void   AddClient(std::shared_ptr<ClientSocket>& pClient)  //���ͻ��˼��뵽�ͻ������ӻ��������
	{
		//�Խ���
		std::lock_guard<std::mutex> lock(_mutex);
		//_mutex.lock(); ��ȻҲ����ʹ�û�����
		_clientsBuff.push_back(pClient);
		//_mutex.unlock();
	}
	int    RecvData(std::shared_ptr<ClientSocket>& pClient);     //��������
	void   OnNetMessage(std::shared_ptr<ClientSocket>& pClient, DataHeader* header);//����������Ϣ
	void   Start() {
		//������ǰ�����߳�
		//����һ���̣߳��߳�ִ�к���ΪOnrun()����ʵ���Բ�����this������Ϊ�˸���ȫ�����Դ���this��Onrun()
		_pthread = new std::thread(std::mem_fn(&CellServer::Onrun), this);

		//�����������
		_taskServer.Start();
	}
public:
	void addSendTask(std::shared_ptr<ClientSocket>& pClient, std::shared_ptr<DataHeader>& header)
	{
		auto task = std::make_shared<CellSendMsg2ClientTask>(pClient, header);
		_taskServer.addTask((CellTaskPtr&)task);
	}
private:
	SOCKET                     _sock;         //����˵��׽���
	std::map<SOCKET, std::shared_ptr<ClientSocket>> _clients; //�����洢�ͻ���
	std::vector<std::shared_ptr<ClientSocket>> _clientsBuff;  //�洢�ͻ������ӻ�����У�֮��ᱻ���뵽_clients��ȥ
	SOCKET                     maxSock;       //��ǰ�����ļ�������ֵ��select�Ĳ���1Ҫʹ��
											  //char _recvBuff[RECV_BUFF_SIZE];           //���ջ�����
	std::mutex                 _mutex;        //������
	std::thread*               _pthread;      //��ǰ�ӷ����ִ�е��߳�
	INetEvent*                 _pNetEvent;
	fd_set                     _fdRead_bak;   //�������浱ǰ��fd_set
	bool                       _clients_change;//��ǰ�Ƿ����¿ͻ��˼������
private:
	CellTaskServer _taskServer;
};

//����������
class EasyTcpServer :public INetEvent
{
public:
	EasyTcpServer() :_sock(INVALID_SOCKET), _msgCount(0), _recvCount(0), _clientCount(0) {}
	virtual ~EasyTcpServer() { CloseSocket(); }
public:
	void InitSocket();  //��ʼ��socket
	int  Bind(const char* ip, unsigned short port);    //�󶨶˿ں�
	int  Listen(int n); //�����˿ں�
	SOCKET Accept();    //���տͻ�������
	void addClientToCellServer(std::shared_ptr<ClientSocket>& pClient);//���¿ͻ����뵽CellServer�Ŀͻ������ӻ��������
	void Start(int nCellServer);                      //�����ӷ����������������еĴӷ�������(����Ϊ�ӷ�����������)
	void CloseSocket(); //�ر�socket
	bool isRun() { return _sock != INVALID_SOCKET; }  //�жϵ�ǰ������Ƿ�������
	bool Onrun();       //����������Ϣ

	void time4msg();    //ÿ1��ͳ��һ���յ������ݰ�������
public:
	//�ͻ��˼����¼�(������̰߳�ȫ�ģ���Ϊ��ֻ�ᱻ��������(�Լ�)����)
	virtual void OnClientJoin(std::shared_ptr<ClientSocket>& pClient)override { _clientCount++; }
	
	//�ͻ����뿪�¼������������������Ƚϼ򵥣�ֻ�ǽ���ǰ�ͻ��˵�����--������ӷ�������ֹһ������ô�˺��������̰߳�ȫ�ģ���Ϊ��������ᱻ����ӷ��������õģ�
	virtual void OnClientLeave(std::shared_ptr<ClientSocket>& pClient)override { _clientCount--; }
	
	//���յ��ͻ�����Ϣ�¼��������ݰ����������ӣ�����ӷ�������ֹһ������ô�˺��������̰߳�ȫ�ģ���Ϊ��������ᱻ����ӷ��������õģ�
	//����1��������һ��CellServer�����������Ϣ��
	virtual void OnNetMsg(CellServer* pCellServer, std::shared_ptr<ClientSocket>& pClient, DataHeader* header)override { _msgCount++; }
	
	//recv�¼�
	virtual void OnNetRecv(std::shared_ptr<ClientSocket>& pClient)override { _recvCount++; }
private:
	SOCKET                     _sock;        //������׽���
	std::vector<std::shared_ptr<CellServer>>   _cellServers; //��Ŵӷ���˶���
	CELLTimestamp              _tTime;       //��ʱ��
	std::atomic_int            _clientCount; //�ͻ��˵�����(�������һ��ԭ�Ӳ�����ûʲô����ԭ��ʹ���������,��ͬ)
	std::atomic_int            _msgCount;    //��ʾ����˽��յ��ͻ������ݰ�������
	std::atomic_int            _recvCount;   //recv()����ִ�еĴ���
};

int ClientSocket::SendData(std::shared_ptr<DataHeader>& header) {
	int ret = SOCKET_ERROR;

	//Ҫ���͵����ݵĳ���
	int nSendLen = header->dataLength;
	const char* pSendData = (const char*)header.get();

	//�������һ��if֮�����nSendLen��Ȼ����SEND_BUFF_SIZE����ô����Ҫ����ִ��if��������������
	while (true)
	{
		//�����ǰ"Ҫ���͵����ݵĳ���+���������ݽ�βλ��"֮���ܵĻ�������С��˵�����������ˣ���ô�����������������ͳ�ȥ
		if (_lastSendPos + nSendLen >= SEND_BUFF_SIZE)
		{
			//���㻺������ʣ���ٿռ�
			int nCopyLen = SEND_BUFF_SIZE - _lastSendPos;
			//Ȼ��nCopyLen���ȵ�pSendData���ݿ�������������
			memcpy(_sendMsgBuff + _lastSendPos, pSendData, nCopyLen);
			//����ʣ������λ��
			pSendData += nCopyLen;
			//����ʣ�����ݳ���
			nSendLen -= nCopyLen;
			ret = send(_sock, _sendMsgBuff, SEND_BUFF_SIZE, 0);
			//����������β����Ϊ0
			_lastSendPos = 0;

			if (ret = SOCKET_ERROR)
				return -1;
		}
		//������ͻ�������û������ô��������Ϣ�ŵ��������У�����ֱ�ӷ���
		else
		{
			memcpy(_sendMsgBuff + _lastSendPos, pSendData, nSendLen);
			//���»���������β��
			_lastSendPos += nSendLen;
			//ֱ���˳���������while
			break;
		}
	}

	return ret;
}

void EasyTcpServer::InitSocket()
{
#ifdef _WIN32
	WORD ver = MAKEWORD(2, 2);
	WSADATA dat;
	WSAStartup(ver, &dat);
#endif

	//����socket
	_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (INVALID_SOCKET == _sock) {
		std::cout << "Server������socket�ɹ�" << std::endl;
	}
	else {
		std::cout << "Server������socket�ɹ�" << std::endl;
	}
}

int EasyTcpServer::Bind(const char* ip, unsigned short port)
{
	if (!isRun())
		InitSocket();

	//��ʼ������˵�ַ
	struct sockaddr_in _sin = {};
#ifdef _WIN32
	if (ip)
		_sin.sin_addr.S_un.S_addr = inet_addr(ip);
	else
		_sin.sin_addr.S_un.S_addr = INADDR_ANY;
#else
	if (ip)
		_sin.sin_addr.s_addr = inet_addr(ip);
	else
		_sin.sin_addr.s_addr = INADDR_ANY;
#endif
	_sin.sin_family = AF_INET;
	_sin.sin_port = htons(port);

	//�󶨷���˵�ַ
	int ret = ::bind(_sock, (struct sockaddr*)&_sin, sizeof(_sin));
	if (SOCKET_ERROR == ret) {
		if (ip)
			std::cout << "Server���󶨵�ַ(" << ip << "," << port << ")ʧ��!" << std::endl;
		else
			std::cout << "Server���󶨵�ַ(INADDR_ANY," << port << ")ʧ��!" << std::endl;
	}
	else {
		if (ip)
			std::cout << "Server���󶨵�ַ(" << ip << "," << port << ")�ɹ�!" << std::endl;
		else
			std::cout << "Server���󶨵�ַ(INADDR_ANY," << port << ")�ɹ�!" << std::endl;
	}
	return ret;
}

void EasyTcpServer::CloseSocket()
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
	}
}

int EasyTcpServer::Listen(int n)
{
	//��������˿�
	int ret = listen(_sock, n);
	if (SOCKET_ERROR == ret)
		std::cout << "Server����������˿�ʧ��!" << std::endl;
	else
		std::cout << "Server����������˿ڳɹ�!" << std::endl;
	return ret;
}

SOCKET EasyTcpServer::Accept()
{
	//��������ͻ��˵�ַ
	struct sockaddr_in _clientAddr = {};
	int nAddrLen = sizeof(_clientAddr);
	SOCKET _cSock = INVALID_SOCKET;

	//���տͻ�������
#ifdef _WIN32
	_cSock = accept(_sock, (struct sockaddr*)&_clientAddr, &nAddrLen);
#else
	_cSock = accept(_sock, (struct sockaddr*)&_clientAddr, (socklen_t*)&nAddrLen);
#endif
	if (INVALID_SOCKET == _cSock) {
		std::cout << "Server�����յ���Ч�ͻ���!" << std::endl;
	}
	else {
		//֪ͨ�����Ѵ��ڵ����пͻ��ˣ����µĿͻ��˼���
		//NewUserJoin newUserInfo(static_cast<int>(_cSock));
		//SendDataToAll(&newUserInfo);

		//�������ӵĿͻ��˼��뵽�ӷ������Ŀͻ��˻��������
		std::shared_ptr<ClientSocket> newClient = std::make_shared<ClientSocket>(_cSock);
		addClientToCellServer(newClient);
		OnClientJoin(newClient); //��Ӧ�ͻ��˼����¼����亯���Ὣ�ͻ��˵�����++

		//std::cout << "Server�����ܵ��µĿͻ���(" << _clients.size() << ")����,IP=" << inet_ntoa(_clientAddr.sin_addr)
		//	<< ",Socket=" << static_cast<int>(_cSock) << std::endl;
	}
	return _cSock;
}

void EasyTcpServer::addClientToCellServer(std::shared_ptr<ClientSocket>& pClient)
{
	//��_cellServers��Ѱ�ң���һ��CellServer�䴦��Ŀͻ��������٣���ô�ͽ��¿ͻ����뵽���CellServer������ȥ
	auto pMinServer = _cellServers[0];
	for (auto pCellServer : _cellServers)
	{
		if (pMinServer->getClientCount() > pCellServer->getClientCount())
		{
			pMinServer = pCellServer;
		}
	}
	pMinServer->AddClient(pClient);
}

void EasyTcpServer::Start(int nCellServer)
{
	for (int i = 0; i < nCellServer; ++i)
	{
		auto ser = std::make_shared<CellServer>(_sock);
		_cellServers.push_back(ser);
		ser->setEventObj(this);
		ser->Start();
	}
}

bool EasyTcpServer::Onrun()
{
	if (isRun())
	{
		time4msg(); //ͳ�Ƶ�ǰ���յ������ݰ�������

		fd_set fdRead;
		//fd_set fdWrite;
		//fd_set fdExp;
		FD_ZERO(&fdRead);
		//FD_ZERO(&fdWrite);
		//FD_ZERO(&fdExp);
		FD_SET(_sock, &fdRead);
		//FD_SET(_sock, &fdWrite);
		//FD_SET(_sock, &fdExp);

		struct timeval t = { 0,0 };
		int ret = select(_sock + 1, &fdRead, nullptr, nullptr, &t);
		if (ret < 0)
		{
			std::cout << "Server��select����" << std::endl;
			//select������ô�Ͳ����ټ�������select������֮�󣬵���CloseSocket()��
			//�ر����з���ˡ��������л����׽��֣���ôisRun()�ͻ᷵��false���Ӷ���ֹserver.cpp��������
			CloseSocket();
			return false;
		}
		if (FD_ISSET(_sock, &fdRead))//���һ���ͻ������ӽ�������ô����˵�socket�ͻ��Ϊ�ɶ��ģ���ʱ����ʹ��accept����������ͻ���
		{
			FD_CLR(_sock, &fdRead);
			Accept();
			return true;
		}
		return true;
	}
	return false;
}

void EasyTcpServer::time4msg()
{
	auto t = _tTime.getElapsedSecond();
	if (t >= 1.0)
	{
		//msgCount,_recvCountΪʲôҪ����t��
		//��Ϊ����Ҫÿ1���Ӵ�ӡһ�ν��յ������ݰ����������������õ�ʱ��ʱ������1�룬��ô���Խ�recvCount/t����ñȽ�ƽ�������ݰ�����/recvִ�д���
		printf("time<%lf>,thread numer<%d>,client number<%d>,msgCount<%d>,recvCount<%d>\n",
			t, _cellServers.size(), static_cast<int>(_clientCount), static_cast<int>(_msgCount / t), static_cast<int>(_recvCount / t));
		_recvCount = 0;
		_msgCount = 0;
		_tTime.update();
	}
}

void CellServer::CloseSocket()
{
	if (_sock != INVALID_SOCKET)
	{
#ifdef _WIN32
		//�����еĿͻ����׽��ֹر�
		for (auto iter : _clients)
		{
			closesocket(iter.second->sockfd());
		}
		//�رշ�����׽���
		closesocket(_sock);

		//��Ϊ���Ǵӷ����������ԾͲ�Ҫ�����׽��ֻ����ˣ����������������׽��ֻ��������
		//WSACleanup();
#else
		for (auto iter : _clients)
		{
			close(iter.second->sockfd());
		}
		close(_sock);
#endif
		_clients.clear();
		_sock = INVALID_SOCKET;
		delete _pthread;
	}
}

bool CellServer::Onrun()
{
	while (isRun())
	{
		//����ͻ��˻������_clientsBuff�����¿ͻ�����ô�ͽ�����뵽_clients��
		if (_clientsBuff.size() > 0)
		{
			//�Խ���lock_guard��������������Զ��ͷ��������ifִ�н���֮��_mutex�ͱ��ͷ���
			std::lock_guard<std::mutex> lock(_mutex);
			for (auto pClient : _clientsBuff)
			{
				_clients[pClient->sockfd()] = pClient;
			}
			_clientsBuff.clear();
			_clients_change = true;
		}

		//���û�пͻ�����ô����һ��Ȼ�������һ��ѭ��
		if (_clients.empty())
		{
			std::chrono::milliseconds t(1);
			std::this_thread::sleep_for(t);
			continue;
		}

		fd_set fdRead;
		FD_ZERO(&fdRead);
		//�����̵߳�select���Ѿ���������˵�_sock���в�ѯ�ˣ����Դӷ������Ͳ���Ҫ�ٽ�_sock���뵽fd_set���ˣ����������ط�ͬʱ���������
		//FD_SET(_sock, &fdRead);

		//����_fdRead_change�ж��Ƿ����¿ͻ��˼��룬�������ô�ͽ����µ�FD_SET
		if (_clients_change)
		{
			_clients_change = false;
			for (auto iter : _clients)
			{
				FD_SET(iter.second->sockfd(), &fdRead);
				if (maxSock < iter.second->sockfd())
					maxSock = iter.second->sockfd();
			}
			//�����º��fd_set���浽_fdRead_bak��
			memcpy(&_fdRead_bak, &fdRead, sizeof(_fdRead_bak));
		}
		//����ֱ�ӿ�����������ѭ��FD_SET��
		else
			memcpy(&fdRead, &_fdRead_bak, sizeof(_fdRead_bak));


		//�ӷ�����һ��ֻ������ȡ���ݣ�������������Ϊ������Ҳ����
		//struct timeval t = { 1,0 };
		int ret = select(maxSock + 1, &fdRead, nullptr, nullptr, nullptr);
		if (ret < 0)
		{
			std::cout << "Server��select����" << std::endl;
			CloseSocket();
			return false;
		}

#ifdef _WIN32
		//�����WIN�����У�fd_setӵ��fd_count��fd_array��Ա
		//���ǿ��Ա���fd_set��Ȼ����л�ȡ���ݣ�����Ҫʹ��FD_ISSET��
		for (int n = 0; n < fdRead.fd_count; n++)
		{
			auto iter = _clients.find(fdRead.fd_array[n]);
			//���RecvData������ô�ͽ��ÿͻ��˴�_client���Ƴ�
			if (-1 == RecvData(iter->second))
			{
				if (_pNetEvent)
					_pNetEvent->OnClientLeave(iter->second); //֪ͨ���������пͻ����˳�
				//delete iter->second;
				_clients.erase(iter->first);
				_clients_change = true; //���Ҫ����Ϊtrue����Ϊ�пͻ����˳��ˣ���Ҫ���½���FD_SET
			}
		}
#else
		//�����UNIX�£�fd_set��fd_count��fd_array��Ա������ֻ�ܱ���_clients����
		//����_clients map���������еĿͻ��ˣ�Ȼ����л�ȡ����
		for (auto iter : _clients)
		{
			//��Ϊ_clients��һ��map�����ÿ��iter����һ��pair,��first��ԱΪkey(SOCKET)��value��ԱΪvalue(ClientSocket)
			if (FD_ISSET(iter.second->sockfd(), &fdRead))
			{
				//���RecvData������ô�ͽ��ÿͻ��˴�_client���Ƴ�
				if (-1 == RecvData(iter.second))
				{
					if (_pNetEvent)
						_pNetEvent->OnClientLeave(iter.second); //֪ͨ���������пͻ����˳�
					_clients.erase(iter.first);
					_clients_change = true; //ԭ��ͬ��
				}
			}
		}
#endif // _WIN32

	}
	return false;
}

int CellServer::RecvData(std::shared_ptr<ClientSocket>& pClient)
{
	char *_recvBuff = pClient->recvMsgBuff() + pClient->getRecvLastPos();
	//�Ƚ����ݽ��յ�_recvBuff��������
	int _nLen = recv(pClient->sockfd(), _recvBuff, RECV_BUFF_SIZE - pClient->getRecvLastPos(), 0);
	_pNetEvent->OnNetRecv(pClient);
	if (_nLen < 0) {
		//std::cout << "recv��������" << std::endl;
		return -1;
	}
	else if (_nLen == 0) {
		//std::cout << "�ͻ���<Socket=" << pClient->sockfd() << ">�����˳���" << std::endl;
		return -1;
	}

	//������Ҫ��һ���ˣ������ݴ�_recvBuff�п������ͻ��˵Ļ�������
	//memcpy(pClient->msgBuff() + pClient->getLastPos(), _recvBuff, _nLen);

	pClient->setRecvLastPos(pClient->getRecvLastPos() + _nLen);
	//�жϿͻ��˵Ļ����������ݽ�β��λ�ã������һ��DataHeader�Ĵ�С��ô�Ϳ��Զ����ݽ��д���
	while (pClient->getRecvLastPos() >= sizeof(DataHeader))
	{
		//��ȡ����������ָ��
		DataHeader* header = (DataHeader*)pClient->recvMsgBuff();
		//�����ǰ�����������ݽ�β��λ�ô��ڵ���һ�����ݰ��Ĵ�С����ô�Ͷ����ݽ��д���
		if (pClient->getRecvLastPos() >= header->dataLength)
		{
			//�ȱ���ʣ��δ������Ϣ�������ĳ���
			int nSize = pClient->getRecvLastPos() - header->dataLength;
			//����������Ϣ
			OnNetMessage(pClient, header);
			//�������֮�󣬽�_recvMsgBuff��ʣ��δ�����ֵ�����ǰ��
			memcpy(pClient->recvMsgBuff(), pClient->recvMsgBuff() + header->dataLength, nSize);
			pClient->setRecvLastPos(nSize);
		}
		else {
			//��Ϣ������ʣ�����ݲ���һ��������Ϣ
			break;
		}
	}
	return 0;
}

void CellServer::OnNetMessage(std::shared_ptr<ClientSocket>& pClient, DataHeader* header)
{
	//����������OnNetMsg�¼�
	_pNetEvent->OnNetMsg(this, pClient, header);

	switch (header->cmd)
	{
	case CMD_LOGIN: //����ǵ�¼
	{
		//Login *login = (Login*)header;
		//std::cout << "����ˣ��յ��ͻ���<Socket=" << pClient->sockfd() << ">����ϢCMD_LOGIN���û�����" << login->userName << "�����룺" << login->PassWord << std::endl;

		//�˴������ж��û��˻��������Ƿ���ȷ�ȵȣ�ʡ�ԣ�

		//���ص�¼�Ľ�����ͻ���
		auto ret = std::make_shared<LoginResult>();
		//��_taskServer()�ڲ���װһ��������Ϣ����Ȼ��ִ�и÷�������
		this->addSendTask(pClient, (std::shared_ptr<DataHeader>)ret);
	}
	break;
	case CMD_LOGOUT:  //������˳�
	{
		//Logout *logout = (Logout*)header;
		//std::cout << "����ˣ��յ��ͻ���<Socket=" << pClient->sockfd() << ">����ϢCMD_LOGOUT���û�����" << logout->userName << std::endl;

		//�����˳��Ľ�����ͻ���
		auto ret = std::make_shared<LogoutResult>();
		this->addSendTask(pClient, (std::shared_ptr<DataHeader>)ret);
	}
	break;
	default:  //����д���
	{
		//std::cout << "����ˣ��յ��ͻ���<Socket=" << pClient->sockfd() << ">��δ֪��Ϣ��Ϣ" << std::endl;

		//���ش�����ͻ��ˣ�DataHeaderĬ��Ϊ������Ϣ
		auto ret = std::make_shared<DataHeader>();
		this->addSendTask(pClient, ret);
	}
	break;
	}
}

#endif