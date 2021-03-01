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
	if (memory_) {
		free(memory_);
	}
}

void MemoryPool::Init(uint32_t size, uint32_t n)
{
	if (memory_) {
		return;
	}

	block_size_ = size;
	num_blocks_ = n;
	memory_ = (char*)malloc(num_blocks_ * (block_size_ + sizeof(MemoryBlock)));
	head_ = (MemoryBlock*)memory_;
	head_->block_id = 1;
	head_->pool = this;
	head_->next = nullptr;

	MemoryBlock* current = head_;
	for (uint32_t n = 1; n < num_blocks_; n++) {
		MemoryBlock* next = (MemoryBlock*)(memory_ + (n * (block_size_ + sizeof(MemoryBlock))));
		next->block_id = n + 1;
		next->pool = this;
		next->next = nullptr;

		current->next = next;
		current = next;
	}
}

void* MemoryPool::Alloc(uint32_t size)
{
	std::lock_guard<std::mutex> locker(mutex_);
	if (head_ != nullptr) {
		MemoryBlock* block = head_;
		head_ = head_->next;
		return ((char*)block + sizeof(MemoryBlock));
	}

	return nullptr;
}

void MemoryPool::Free(void* ptr)
{
	MemoryBlock *block = (MemoryBlock*)((char*)ptr - sizeof(MemoryBlock));
	if (block->block_id != 0) {
		std::lock_guard<std::mutex> locker(mutex_);
		block->next = head_;
		head_ = block;
	}
}

MemoryManager::MemoryManager()
{
	memory_pools_[0].Init(4096, 50);
	memory_pools_[1].Init(40960, 10);
	memory_pools_[2].Init(102400, 5);
	//memory_pools_[3].Init(204800, 2);
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
	for (int n = 0; n < kMaxMemoryPool; n++) {
		if (size <= memory_pools_[n].BolckSize()) {
			void* ptr = memory_pools_[n].Alloc(size);
			if (ptr != nullptr) {
				return ptr;
			}				
			else {
				break;
			}
		}
	} 

	MemoryBlock *block = (MemoryBlock*)malloc(size + sizeof(MemoryBlock));
	block->block_id = 0;
	block->pool = nullptr;
	block->next = nullptr;
	return ((char*)block + sizeof(MemoryBlock));
}

void MemoryManager::Free(void* ptr)
{
	MemoryBlock *block = (MemoryBlock*)((char*)ptr - sizeof(MemoryBlock));
	MemoryPool *pool = block->pool;
	
	if (pool != nullptr && block->block_id > 0) {
		pool->Free(ptr);
	}
	else {
		::free(block);
	}
}
