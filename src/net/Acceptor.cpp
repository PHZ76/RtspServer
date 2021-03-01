#include "Acceptor.h"
#include "EventLoop.h"
#include "SocketUtil.h"
#include "Logger.h"

using namespace xop;

Acceptor::Acceptor(EventLoop* eventLoop)
    : event_loop_(eventLoop)
    , tcp_socket_(new TcpSocket)
{	
	
}

Acceptor::~Acceptor()
{

}

int Acceptor::Listen(std::string ip, uint16_t port)
{
	std::lock_guard<std::mutex> locker(mutex_);

	if (tcp_socket_->GetSocket() > 0) {
		tcp_socket_->Close();
	}

	SOCKET sockfd = tcp_socket_->Create();
	channel_ptr_.reset(new Channel(sockfd));
	SocketUtil::SetReuseAddr(sockfd);
	SocketUtil::SetReusePort(sockfd);
	SocketUtil::SetNonBlock(sockfd);

	if (!tcp_socket_->Bind(ip, port)) {
		return -1;
	}

	if (!tcp_socket_->Listen(1024)) {
		return -1;
	}

	channel_ptr_->SetReadCallback([this]() { this->OnAccept(); });
	channel_ptr_->EnableReading();
	event_loop_->UpdateChannel(channel_ptr_);
	return 0;
}

void Acceptor::Close()
{
	std::lock_guard<std::mutex> locker(mutex_);

	if (tcp_socket_->GetSocket() > 0) {
		event_loop_->RemoveChannel(channel_ptr_);
		tcp_socket_->Close();
	}
}

void Acceptor::OnAccept()
{
	std::lock_guard<std::mutex> locker(mutex_);

	SOCKET socket = tcp_socket_->Accept();
	if (socket > 0) {
		if (new_connection_callback_) {
			new_connection_callback_(socket);
		}
		else {
			SocketUtil::Close(socket);
		}
	}
}

