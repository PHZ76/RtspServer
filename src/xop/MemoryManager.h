#ifndef XOP_MEMMORY_MANAGER_H
#define XOP_MEMMORY_MANAGER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <mutex>

namespace xop
{

void* Alloc(uint32_t size);
void Free(void *ptr);

class MemoryPool;
struct MemoryBlock
{
	uint32_t _blockId = 0;
	MemoryPool *_pool = nullptr;
	MemoryBlock *_next = nullptr;
};

class MemoryPool
{
public:
	MemoryPool();
	~MemoryPool();

	void Init(uint32_t size, uint32_t n);
	void* Alloc(uint32_t size);
	void Free(void* ptr);

	size_t BolckSize() const
	{
		return _blockSize;
	}

//private:
	char* _memory = nullptr;
	uint32_t _blockSize = 0;
	uint32_t _numBlocks = 0;
	MemoryBlock* _head = nullptr;
	std::mutex _mutex;
};

class MemoryManager
{
public:
	static MemoryManager& Instance();
	~MemoryManager();

	void* Alloc(uint32_t size);
	void Free(void* ptr);

private:
	MemoryManager();

	static const int kMaxMemoryPool = 3;
	MemoryPool _memoryPools[kMaxMemoryPool];
};

}
#endif
