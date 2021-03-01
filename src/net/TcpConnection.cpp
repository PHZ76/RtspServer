#include "TcpConnection.h"
#include "SocketUtil.h"

using namespace xop;

TcpConnection::TcpConnection(TaskScheduler *task_scheduler, SOCKET sockfd)
	: task_scheduler_(task_scheduler)
	, read_buffer_(new BufferReader)
	, write_buffer_(new BufferWriter(500))
	, channel_(new Channel(sockfd))
{
	is_closed_ = false;

	channel_->SetReadCallback([this]() { this->HandleRead(); });
	channel_->SetWriteCallback([this]() { this->HandleWrite(); });
	channel_->SetCloseCallback([this]() { this->HandleClose(); });
	channel_->SetErrorCallback([this]() { this->HandleError(); });

	SocketUtil::SetNonBlock(sockfd);
	SocketUtil::SetSendBufSize(sockfd, 100 * 1024);
	SocketUtil::SetKeepAlive(sockfd);

	channel_->EnableReading();
	task_scheduler_->UpdateChannel(channel_);
}

TcpConnection::~TcpConnection()
{
	SOCKET fd = channel_->GetSocket();
	if (fd > 0) {
		SocketUtil::Close(fd);
	}
}

void TcpConnection::Send(std::shared_ptr<char> data, uint32_t size)
{
	if (!is_closed_) {
		mutex_.lock();
		write_buffer_->Append(data, size);
		mutex_.unlock();

		this->HandleWrite();
	}
}

void TcpConnection::Send(const char *data, uint32_t size)
{
	if (!is_closed_) {
		mutex_.lock();
		write_buffer_->Append(data, size);
		mutex_.unlock();

		this->HandleWrite();
	}
}

void TcpConnection::Disconnect()
{
	std::lock_guard<std::mutex> lock(mutex_);
	auto conn = shared_from_this();
	task_scheduler_->AddTriggerEvent([conn]() {
		conn->Close();
	});
}

void TcpConnection::HandleRead()
{
	{
		std::lock_guard<std::mutex> lock(mutex_);

		if (is_closed_) {
			return;
		}
		
		int ret = read_buffer_->Read(channel_->GetSocket());
		if (ret <= 0) {
			this->Close();
			return;
		}
	}

	if (read_cb_) {
		bool ret = read_cb_(shared_from_this(), *read_buffer_);
		if (false == ret) {
			std::lock_guard<std::mutex> lock(mutex_);
			this->Close();
		}
	}
}

void TcpConnection::HandleWrite()
{
	if (is_closed_) {
		return;
	}
	
	//std::lock_guard<std::mutex> lock(mutex_);
	if (!mutex_.try_lock()) {
		return;
	}

	int ret = 0;
	bool empty = false;
	do
	{
		ret = write_buffer_->Send(channel_->GetSocket());
		if (ret < 0) {
			this->Close();
			mutex_.unlock();
			return;
		}
		empty = write_buffer_->IsEmpty();
	} while (0);

	if (empty) {
		if (channel_->IsWriting()) {
			channel_->DisableWriting();
			task_scheduler_->UpdateChannel(channel_);
		}
	}
	else if(!channel_->IsWriting()) {
		channel_->EnableWriting();
		task_scheduler_->UpdateChannel(channel_);
	}

	mutex_.unlock();
}

void TcpConnection::Close()
{
	if (!is_closed_) {
		is_closed_ = true;
		task_scheduler_->RemoveChannel(channel_);

		if (close_cb_) {
			close_cb_(shared_from_this());
		}			

		if (disconnect_cb_) {
			disconnect_cb_(shared_from_this());
		}	
	}
}

void TcpConnection::HandleClose()
{
	std::lock_guard<std::mutex> lock(mutex_);
	this->Close();
}

void TcpConnection::HandleError()
{
	std::lock_guard<std::mutex> lock(mutex_);
	this->Close();
}
