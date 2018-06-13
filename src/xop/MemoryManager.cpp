#include "MemoryManager.h"

using namespace xop;

void* xop::Alloc(uint32_t size)
{
	return MemoryManager::Instance().Alloc(size);
}

void xop::Free(void *ptr)
{
	return MemoryManager::Instance().Free(ptr);
}

MemoryPool::MemoryPool()
{

}

MemoryPool::~MemoryPool()
{
	if (_memory)
		free(_memory);
}

void MemoryPool::Init(uint32_t size, uint32_t n)
{
	if (_memory)
		return;

	_blockSize = size;
	_numBlocks = n;
	_memory = (char*)malloc(_numBlocks * (_blockSize + sizeof(MemoryBlock)));
	_head = (MemoryBlock*)_memory;
	_head->_blockId = 1;
	_head->_pool = this;
	_head->_next = nullptr;

	MemoryBlock* current = _head;
	for (uint32_t n = 1; n < _numBlocks; n++)
	{
		MemoryBlock* next = (MemoryBlock*)(_memory + (n * (_blockSize + sizeof(MemoryBlock))));
		next->_blockId = n + 1;
		next->_pool = this;
		next->_next = nullptr;

		current->_next = next;
		current = next;
	}
}

void* MemoryPool::Alloc(uint32_t size)
{
	MemoryBlock* block = nullptr;

	std::lock_guard<std::mutex> locker(_mutex);
	if (_head != nullptr)
	{
		MemoryBlock* block = _head;
		_head = _head->_next;
		return ((char*)block + sizeof(MemoryBlock));
	}

	return nullptr;
}

void MemoryPool::Free(void* ptr)
{
	MemoryBlock *block = (MemoryBlock*)((char*)ptr - sizeof(MemoryBlock));
	if (block->_blockId != 0)
	{
		std::lock_guard<std::mutex> locker(_mutex);
		block->_next = _head;
		_head = block;
	}
}

MemoryManager::MemoryManager()
{
	_memoryPools[0].Init(4096, 50);
	_memoryPools[1].Init(40960, 10);
	_memoryPools[2].Init(102400, 5);
	//_memoryPools[3].Init(204800, 2);
}

MemoryManager::~MemoryManager()
{

}

MemoryManager& MemoryManager::Instance()
{
	static MemoryManager s_mgr;
	return s_mgr;
}

void* MemoryManager::Alloc(uint32_t size)
{
	for (int n = 0; n < kMaxMemoryPool; n++)
	{
		if (size <= _memoryPools[n].BolckSize())
		{
			void* ptr = _memoryPools[n].Alloc(size);
			if (ptr != nullptr)
				return ptr;
			else
				break;
		}
	}

	MemoryBlock *block = (MemoryBlock*)malloc(size + sizeof(MemoryBlock));
	block->_blockId = 0;
	block->_pool = nullptr;
	block->_next = nullptr;
	return ((char*)block + sizeof(MemoryBlock));
}

void MemoryManager::Free(void* ptr)
{
	MemoryBlock *block = (MemoryBlock*)((char*)ptr - sizeof(MemoryBlock));
	MemoryPool *pool = block->_pool;
	
	if (pool != nullptr && block->_blockId > 0)
	{
		pool->Free(ptr);
	}
	else
	{
		::free(block);
	}
}