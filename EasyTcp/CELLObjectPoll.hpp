#ifndef __CELLOBJECTPOLL_HPP__
#define __CELLOBJECTPOLL_HPP__

#ifdef _DEBUG
#include <stdio.h>
#define xPrintf(...) printf(__VA_ARGS__)
#else
#define xPrintf(...)
#endif // _DEBUG

#include <stdio.h>
#include <mutex>

template<typename Type,size_t nPoolSize>
class CELLObjectPoll
{
public:
	CELLObjectPoll() {

	}
	~CELLObjectPoll(){
		
	}
public:
	//�ͷŶ���
	//�������
	void *allocObjMemory(size_t nSize)
	{
		//����
		std::lock_guard<std::mutex> lg(_mutex);

		//����ڴ��δ��ʼ������ô���г�ʼ��
		if (!_pBuf)
			initMemory();

		NodeHeader* pReturn = nullptr;

		//����ڴ��ʹ�����ˣ���ô_pHeaderӦ��ָ��ĳ���ڴ������nullptrβָ��
		//��ʱ����malloc��ϵͳ�����ڴ棬�������ǵ��ڴ�ؽ����ڴ�����
		if (_pHeader == nullptr) {
			pReturn = (NodeHeader*)new char[sizeof(Type) + sizeof(NodeHeader)];
			pReturn->bPoll = false;
			pReturn->nID = -1;
			pReturn->nRef = 1;
			pReturn->pNext = nullptr;
		}
		else {
			//�����ʱ��_pHeader����ƶ�
			pReturn = _pHeader;
			_pHeader = _pHeader->pNext;
			pReturn->nRef = 1;
		}

		xPrintf("allocObjMemory: %llx,id=%d, size=%d\n", pReturn, pReturn->nID, nSize);
		//���ؿ����ڴ�
		return ((char*)pReturn + sizeof(NodeHeader));
	}

	//��ʼ�������
	void initPool()
	{
		if (_pBuf)
			return;
		
		//������е����ڴ浥Ԫ�Ĵ�С
		size_t aSingleSize = sizeof(Type) + sizeof(NodeHeader);

		//������ܵĴ�С
		size_t n = nPoolSize*aSingleSize;
		_pBuf = new char[n];

		
		//��ʼ�������
		//�ȳ�ʼ����һ������Ԫ
		_pHeader = (NodeHeader*)_pBuf;
		_pHeader->nID = 0;
		_pHeader->nRef = 0;
		_pHeader->bPool = true;
		_pHeader->pNext = nullptr;
		//��ʼ��ʣ��Ķ���Ԫ
		NodeHeader* pTemp = _pHeader;
		for (size_t n = 1; n < nPoolSize; ++n)
		{
			NodeHeader* pTempNext = (NodeHeader*)_pBuf + (n*aSingleSize);
			pTempNext->nID = n;
			pTempNext->nRef = 0;
			pTempNext->bPool = true;
			pTempNext->pNext = nullptr;
			pTemp->pNext = pTempNext;
			pTemp = pTempNext;
		}
	}
private:
	class NodeHeader
	{
	public:
		NodeHeader* pNext; //��һ��λ��
		int nID;           //�ڴ����
		char nRef;         //���ô���
		bool bPool;        //�ͷ��ڶ������
	private:
		//���������ã���Ϊ���ڴ���벹�룬x64ƽ̨��NodeHeader�պ�Ϊ16KB
		char c1;
		char c2;
	};
private:
	NodeHeader* _pHeader; //
	char* _pBuf;          //�������ʼ��ַ
	std::mutex _mutex;
};



template<typename Type>
class ObjectPollBase
{
public:
	void* operator new(size_t nSize)
	{
		return malloc(nSize);
	}
	void operator delete(void* p)
	{
		free(p);
	}

	template<typename ...Args>
	static Type* createObject(Args ... args)
	{
		Type* obj = new Type(args...);
		return obj;
	}
	static void destroyObject(Type* obj)
	{
		delete obj;
	}
};


#endif