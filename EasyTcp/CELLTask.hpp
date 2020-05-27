#ifndef __CELLTASK_HPP__
#define __CELLTASK_HPP__

#include <thread>
#include <mutex>
#include <list>
//#include <functional>

using namespace std;

//��������-����
class CellTask
{
public:
	//ִ������
	virtual void doTask() = 0;
};

typedef std::shared_ptr<CellTask> CellTaskPtr;
//ִ������ķ�������
class CellTaskServer
{
public:
	//�������
	void addTask(CellTaskPtr& task)
	{
		std::lock_guard<std::mutex> lock(_mutex);
		_tasksBuff.push_back(task);
	}

	//��������
	void Start()
	{
		std::thread t(std::mem_fn(&CellTaskServer::OnRun), this);
		t.detach();
	}
private:
	//ִ������
	void OnRun();
private:
	std::list<CellTaskPtr> _tasks;     //������������
	std::list<CellTaskPtr> _tasksBuff; //�������ݻ���������
	std::mutex _mutex;               //������
};

void CellTaskServer::OnRun()
{
	while (true)
	{
		//���_tasksBuff��Ϊ�գ���ô���������������������
		if (!_tasksBuff.empty())
		{
			std::lock_guard<std::mutex> lock(_mutex);
			for (auto pTask : _tasksBuff)
			{
				_tasks.push_back(pTask);
			}
			_tasksBuff.clear();
		}

		//�����ǰû����������1�������ѭ��
		if (_tasks.empty())
		{
			std::chrono::milliseconds t(1);
			std::this_thread::sleep_for(t);
			continue;
		}

		//���������ִ������
		for (auto pTask : _tasks)
		{
			pTask->doTask();
		}
		_tasks.clear();
	}
}
#endif