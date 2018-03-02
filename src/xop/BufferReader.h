#ifndef XOP_BUFFER_READER__H
#define XOP_BUFFER_READER__H

#include <cstdint>
#include <vector>
#include <string>
#include <algorithm>  

namespace xop
{
	class BufferReader
	{
	public:	
		static const uint32_t kInitialSize = 2048;
		BufferReader(uint32_t initialSize = kInitialSize);
		~BufferReader();
	
		uint32_t readableBytes() const
		{ 
			return _writerIndex - _readerIndex; 
		}

		uint32_t writableBytes() const
		{ 
			return _buffer.size() - _writerIndex; 
		}
	
		char* peek() 
		{ 
			return begin() + _readerIndex; 
		}
	
		const char* peek() const
		{ return begin() + _readerIndex; }
	
		const char* findFirstCrlf() const
		{    
			const char* crlf = std::search(peek(), beginWrite(), kCRLF, kCRLF+2);
			return crlf == beginWrite() ? nullptr : crlf;
		}
	
		const char* findLastCrlf() const
		{    
			const char* crlf = std::find_end(peek(), beginWrite(), kCRLF, kCRLF+2);
			return crlf == beginWrite() ? nullptr : crlf;
		}
	
		void retrieveAll() 
		{ 
			_writerIndex=0; 
			_readerIndex=0; 
		}
	
		void retrieve(size_t len)
		{
			if (len <= readableBytes())
			{
				_readerIndex += len;
				if(_readerIndex == _writerIndex)
				{
					_readerIndex = 0;
					_writerIndex = 0;
				}
			}
			else
			{
				retrieveAll();
			}
		}

		void retrieveUntil(const char* end)
		{
			retrieve(end - peek());
		}
	
		int readFd(int sockfd);
		uint32_t readAll(std::string& data);
		uint32_t readUntilCrlf(std::string& data);
	
		//bool append();
	
		uint32_t bufferSize() const 
		{ 
			return _buffer.size(); 
		}
	
	private:
		char* begin()
		{ return &*_buffer.begin(); }

		const char* begin() const
		{ return &*_buffer.begin(); }
	
		char* beginWrite()
		{ return begin() + _writerIndex; }

		const char* beginWrite() const
		{ return begin() + _writerIndex; }

		std::vector<char> _buffer;
		size_t _readerIndex = 0;
		size_t _writerIndex = 0;
	
		static const char kCRLF[];
		static const uint32_t MAX_BYTES_PER_READ = 2048;
		static const uint32_t MAX_BUFFER_SIZE = 102400;
	};
}

#endif


