#include "EasyTcpServer.hpp"
#include "MessageHeader.hpp"
#include "Alloctor.h"
#include "CELLObjectPoll.hpp"

/*class MyServer :public EasyTcpServer
{
public:
	//�ͻ��˼����¼�
	virtual void OnClientJoin(ClientSocket* pClient)override {
		_clientCount++;
		printf("client<%d> join\n", pClient->sockfd());
	}
	//�ͻ����뿪�¼�
	virtual void OnClientLeave(ClientSocket* pClient)override {
		_clientCount--;
		printf("client<%d> leave\n", pClient->sockfd());
	}
	//���յ��ͻ�����Ϣ�¼�
	virtual void OnNetMsg(ClientSocket* pClient, DataHeader* header)override 
	{
		_recvCount++;
		switch (header->cmd)
		{
		case CMD_LOGIN: //����ǵ�¼
		{
			//Login *login = (Login*)header;
			//std::cout << "����ˣ��յ��ͻ���<Socket=" << pClient->sockfd() << ">����ϢCMD_LOGIN���û�����" << login->userName << "�����룺" << login->PassWord << std::endl;

			//�˴������ж��û��˻��������Ƿ���ȷ�ȵȣ�ʡ�ԣ�

			//���ص�¼�Ľ�����ͻ���
			LoginResult ret;
			pClient->SendData(&ret);
		}
		break;
		case CMD_LOGOUT:  //������˳�
		{
			//Logout *logout = (Logout*)header;
			//std::cout << "����ˣ��յ��ͻ���<Socket=" << pClient->sockfd() << ">����ϢCMD_LOGOUT���û�����" << logout->userName << std::endl;

			//�����˳��Ľ�����ͻ���
			LogoutResult ret;
			pClient->SendData(&ret);
		}
		break;
		default:  //����д���
		{
			//std::cout << "����ˣ��յ��ͻ���<Socket=" << pClient->sockfd() << ">��δ֪��Ϣ��Ϣ" << std::endl;

			//���ش�����ͻ��ˣ�DataHeaderĬ��Ϊ������Ϣ
			DataHeader ret;
			pClient->SendData(&ret);
		}
		break;
		}
	}
};*/


class ClassA :public ObjectPollBase<ClassA>
{
public:

};


int main()
{
	/*EasyTcpServer server;
	server.Bind("192.168.0.105", 4567);
	server.Listen(5);
	server.Start(4);

	while (server.isRun())
	{
		server.Onrun();
	}

	server.CloseSocket();
	std::cout << "�����ֹͣ����!" << std::endl;

	getchar();  //��ֹ����һ������*/
	return 0;
}