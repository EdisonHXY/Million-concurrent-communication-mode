#ifndef __MEMORYMGR_HPP_
#define __MEMORYMGR_HPP_

#include <assert.h>
#include <stdlib.h>
#include <mutex>


#ifndef  MAX_MEMORY_SIZE
#define  MAX_MEMORY_SIZE 128
#endif // ! MAX_MEMORY_SIZE

#ifdef _DEBUG
	#ifndef xPrintf
		#include <stdio.h>
		#define xPrintf(...) printf(__VA_ARGS__)
	#endif // xPrintf
#else
	#ifndef xPrintf
		#define xPrintf(...)
	#endif
#endif // _DEBUG

class MemoryBlock;
class MemoryAlloc;
class MemoryMgr;

//�����ڴ�����Ϣ���ڴ�������С��Ԫ��
class MemoryBlock
{
	//32�ֽ�
public:
	MemoryAlloc* pAlloc; //�������ڴ��
	MemoryBlock* pNext;  //��һ��λ��
	int nID;             //�ڴ����
	int nRef;            //���ô���
	bool bPoll;          //�ͷ����ڴ����
private:
	//����������Ϊ���ڴ�����
	char c1;
	char c2;
	char c3;
};

//�ڴ��
class MemoryAlloc
{
public:
	MemoryAlloc() {
		_pBuf = nullptr;
		_pHeader = nullptr;
		_nSize = 0;
		_nBlockSize = 0;
	}

	~MemoryAlloc() {
		//ֻ�ͷ�_pBuf�Ϳ�����
		if (!_pBuf)
			delete _pBuf;
		//allocMem����ϵͳ�����ڴ��
	}
public:
	void initMemory()//��ʼ���ڴ��
	{
		xPrintf("initMemory(): _nSize=%d ,_nBlockSize=%d\n", _nSize, _nBlockSize);
		//�����Ϊ�գ�˵���Ѿ���ʼ�����ˣ�ֱ��return
		if (_pBuf)
			return;

		//�ڴ���е����ڴ浥Ԫ�Ĵ�С������MemoryBlock������Ϣ+ʵ�ʴ洢���ݵ�_nSize��С��
		size_t aSingleSize = _nSize + sizeof(MemoryBlock);

		//�ڴ�ص��ܴ�С
		size_t bufSize = aSingleSize*_nBlockSize;
		_pBuf = (char*)malloc(bufSize);

		//��ʼ���ڴ��
		//��ʼ����һ������
		_pHeader = (MemoryBlock*)_pBuf;
		_pHeader->bPoll = true;
		_pHeader->nID = 0;
		_pHeader->nRef = 0;
		_pHeader->pAlloc = this;
		_pHeader->pNext = nullptr;
		//��ʼ��ʣ��������
		MemoryBlock* pTempNext = _pHeader;
		for (size_t n = 1; n < _nBlockSize; ++n)
		{
			MemoryBlock* pTemp = (MemoryBlock*)(_pBuf + (n*aSingleSize));
			pTemp->bPoll = true;
			pTemp->nID = n;
			pTemp->nRef = 0;
			pTemp->pAlloc = this;
			pTemp->pNext = nullptr;
			pTempNext->pNext = pTemp;
			pTempNext = pTemp;
		}
	}
	void *allocMemory(size_t nSize) //�����ڴ�
	{
		//����
		std::lock_guard<std::mutex> lg(_mutex);

		//����ڴ��δ��ʼ������ô���г�ʼ��
		if (!_pBuf)
			initMemory();

		MemoryBlock* pReturn = nullptr;

		//����ڴ��ʹ�����ˣ���ô_pHeaderӦ��ָ��ĳ���ڴ������nullptrβָ��
		//��ʱ����malloc��ϵͳ�����ڴ棬�������ǵ��ڴ�ؽ����ڴ�����
		if (_pHeader == nullptr) {
			pReturn = (MemoryBlock*)malloc(nSize + sizeof(MemoryBlock));
			pReturn->bPoll = false;
			pReturn->nID = -1;
			pReturn->nRef = 1;
			pReturn->pAlloc = nullptr; //����������ڴ洦������������Ϊnullptr
			pReturn->pNext = nullptr;
			printf("MemoryAlloc::allocMem: %llx,id=%d, size=%d\n", pReturn, pReturn->nID, nSize);
		}
		else {
			//�����ʱ��_pHeader����ƶ�
			pReturn = _pHeader;
			_pHeader = _pHeader->pNext;
			//assert(0 == pReturn->nRef);
			pReturn->nRef = 1;
		}

		xPrintf("MemoryAlloc::allocMem: %llx,id=%d, size=%d\n", pReturn, pReturn->nID, nSize);
		//���ؿ����ڴ�
		return ((char*)pReturn + sizeof(MemoryBlock));
	}

	void freeMemory(void *pMem)        //�ͷ��ڴ�
	{
		//�����������ʵ�ʿ��õ��ڴ��ַ����Ҫ��ȥsizeof(MemoryBlock)���ܵõ����������ڴ�������
		MemoryBlock* pBlock = (MemoryBlock*)((char*)pMem - sizeof(MemoryBlock));

		//���������ڴ������ڴ���еģ�����ϵͳ����ģ���ִ��if
		if (pBlock->bPoll)
		{
			//����
			std::lock_guard<std::mutex> lg(_mutex);
			
			//�Ƚ����ڴ������ü�����1��Ȼ���ж����ü����Ƿ����1���������1��ô���ͷ�ֱ��return
			if (--pBlock->nRef != 0)
				return;
			
			//�ͷ��ڴ��ʱ��_pHeaderǰ�ƣ�������ʱ�෴
			pBlock->pNext = _pHeader;
			_pHeader = pBlock;
		}
		else {
			if (--pBlock->nRef != 0)
				return;
			free(pBlock);
		}
	}

protected:
	char*  _pBuf;         //���ڴ�ص���ʼ��ַ
	MemoryBlock* _pHeader;//���ڴ���е�һ���ڴ��
	size_t _nSize;        //���ڴ���е����ڴ��Ĵ�С
	size_t _nBlockSize;   //���ڴ�����ڴ�������

	std::mutex _mutex;
};

template<size_t nSize, size_t nBlockSize>
class MemoryAlloctor :public MemoryAlloc
{
public:
	MemoryAlloctor()
	{
		xPrintf("MemoryAlloctor\n");
		//Ϊ�˷�ֹ�����nSize�����������������Ļ�������ڴ���Ƭ���������ʹ�����㣬����������ֵ��������������ô��ת��Ϊ������
		const size_t n = sizeof(void*);
		_nSize = (nSize / n)*n + (nSize % n ? n : 0);
		_nBlockSize = nBlockSize;
	}
};

//�ڴ�ع�����(����ģʽ)
class MemoryMgr
{
private:
	MemoryMgr() { 
		init_szAlloc(0, 64, &_mem64);
		init_szAlloc(65, 128, &_mem128);
		//init_szAlloc(129, 256, &_mem256);
		//init_szAlloc(257, 512, &_mem512);
		//init_szAlloc(513, 1024, &_mem1024);
		xPrintf("MemoryMgr\n");
	}
	MemoryMgr(const MemoryMgr& rhs) {}
public:
	static MemoryMgr& getInstance()
	{
		static MemoryMgr mgr;
		return mgr;
	}
	void *allocMem(size_t nSize) //�����ڴ�
	{
		if (nSize <= MAX_MEMORY_SIZE)
			return _szAlloc[nSize]->allocMemory(nSize);
		else {
			MemoryBlock* pReturn = (MemoryBlock*)malloc(nSize + sizeof(MemoryBlock));
			pReturn->bPoll = false;
			pReturn->nID = -1;
			pReturn->nRef = 1;
			pReturn->pAlloc = nullptr;
			pReturn->pNext = nullptr;
			xPrintf("MemoryMgr::allocMem: %llx,id=%d, size=%d\n", pReturn, pReturn->nID, nSize);
			return ((char*)pReturn + sizeof(MemoryBlock));
		}
	}
	void freeMem(void *pMem)     //�ͷ��ڴ�
	{
		MemoryBlock* pBlock = (MemoryBlock*)((char*)pMem - sizeof(MemoryBlock));

		xPrintf("MemoryMgr::freeMem: %llx,id=%d\n", pBlock, pBlock->nID);

		//���������ڴ������ڴ���еģ�����ϵͳ����ģ���ִ��if
		if (pBlock->bPoll)
			pBlock->pAlloc->freeMemory(pMem);
		else
		{
			//�Ƚ����ü�����1��Ȼ���ж����ü����Ƿ����0���������0�ͷ��ڴ�
			if (--pBlock->nRef == 0)
				free(pBlock);
		}
	}
	void addRef(void* pMem)      //�����ڴ������ü���
	{
		MemoryBlock* pBlock = (MemoryBlock*)((char*)pMem - sizeof(MemoryBlock));
		++pBlock->nRef;
	}
private:
	void init_szAlloc(int nBegin, int nEnd, MemoryAlloc* pMemA) //��ʼ���ڴ��ӳ������
	{
		for (int n = nBegin; n <= nEnd; n++)
			_szAlloc[n] = pMemA;
	}
private:
	MemoryAlloctor<64, 4000000> _mem64;   //�ڴ�أ����ڴ����100�鵥���ڴ�飬ÿ���ڴ��64KB(��ͬ)
	MemoryAlloctor<128, 1000000> _mem128;
	/*MemoryAlloctor<256, 100> _mem256;
	MemoryAlloctor<512, 100> _mem512;
	MemoryAlloctor<1024, 100> _mem1024;*/
	MemoryAlloc* _szAlloc[MAX_MEMORY_SIZE + 1]; //�����洢�������Щ�ڴ��
};

#endif