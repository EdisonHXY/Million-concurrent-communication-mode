#include "EasyTcpClient.hpp"
#include "CELLTimestamp.hpp"
#include <thread>
#include <atomic>

bool g_bRun = false;
const int cCount = 1000;      //�ͻ��˵�����
const int tCount = 4;          //�̵߳�����
std::atomic_int sendCount = 0; //send()����ִ�еĴ���
std::atomic_int readyCount = 0;//�����Ѿ�׼���������߳�����
EasyTcpClient* client[cCount]; //�ͻ��˵�����

void cmdThread();
void sendThread(int id);

int main()
{
	g_bRun = true;

	//UI�̣߳�������������
	std::thread t(cmdThread);
	t.detach();

	//���������߳�
	for (int n = 0; n < tCount; ++n)
	{
		std::thread t(sendThread, n + 1);
		t.detach();
	}

	//ÿ1���д�ӡһ����Ϣ(���а���send()������ִ�д���)
	CELLTimestamp tTime;
	while (true)
	{
		auto t = tTime.getElapsedSecond();
		if (t >= 1.0)
		{
			printf("time<%lf>,thread numer<%d>,client number<%d>,sendCount<%d>\n",
				t, tCount, cCount, static_cast<int>(sendCount / t));
			sendCount = 0;
			tTime.update();
		}
		Sleep(1);
	}

	return 0;
}

void cmdThread()
{
	char cmdBuf[256] = {};
	while (true)
	{
		std::cin >> cmdBuf;
		if (0 == strcmp(cmdBuf, "exit"))
		{
			g_bRun = false;
			break;
		}
		else {
			std::cout << "���ʶ������������" << std::endl;
		}
	}
}

void sendThread(int id)
{
	/*
		�����⼸��������Ϊ��ƽ��ÿ���̴߳����Ŀͻ��˵�������
			���磬���β���ʱ�ͻ�������Ϊ1000���߳�����Ϊ4����ôÿ���߳�Ӧ�ô���250���ͻ���
				�߳�1��c=250��begin=0,end=250
				�߳�2��c=250��begin=250,end=500
				�߳�3��c=250��begin=500,end=750
				�߳�4��c=250��begin=750,end=1000
	*/
	int c = cCount / tCount;
	int begin = (id - 1)*c;
	int end = id*c;

	for (int n = begin; n < end; ++n) //�����ͻ���
	{
		client[n] = new EasyTcpClient;
	}
	for (int n = begin; n < end; ++n) //��ÿ���ͻ������ӷ�����
	{
		client[n]->ConnectServer("192.168.0.105", 4567);
	}
	printf("Thread<%d>,Connect=<begin=%d, end=%d>\n", id, (begin + 1), end);

	
	//��readyCount��Ȼ���ж�readyCount�Ƿ�ﵽ��tCount
	//���û�У�˵�����е��̻߳�û��׼���ã���ô�͵ȴ������̶߳�׼����һ�𷵻ط�������
	readyCount++;
	while (readyCount < tCount)
	{
		std::chrono::microseconds t(10);
		std::this_thread::sleep_for(t);
	}

	//���ﶨ��Ϊ���飬��������������޸Ŀͻ��˵��η��͸�����˵����ݰ�����
	const int nNum = 10;
	Login login[nNum];
	for (int n = 0; n < nNum; ++n)
	{
		strcpy(login[n].userName, "dongshao");
		strcpy(login[n].PassWord, "123456");
	}
	//�����涨��nLen���Ͳ���ÿ����forѭ����SendDataʱ��Ҫȥsizeof����һ��login�Ĵ�С
	int nLen = sizeof(login);
	//ѭ�������˷�����Ϣ

	while (g_bRun)
	{
		for (int n = begin; n < end; ++n)
		{
			if (client[n]->SendData(login, nLen) != SOCKET_ERROR)
			{
				sendCount++;
			}
			client[n]->Onrun();
		}
		
	}

	//�رտͻ���
	for (int n = begin; n < end; ++n)
	{
		client[n]->CloseSocket();
		delete client[n];
	}

	printf("thread:all clients close the connection!\n");
}