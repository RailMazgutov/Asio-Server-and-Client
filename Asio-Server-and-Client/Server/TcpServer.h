#pragma once

#include <asio/ip/tcp.hpp>
#include <asio/io_service.hpp>

#include <mutex>
#include <map>
#include <queue>

#include "BaseServer.h"

using namespace asio;
class TcpServer : public BaseServer, std::enable_shared_from_this<TcpServer>
{
public:
	TcpServer();
	~TcpServer() = default;
private:
	class TcpClient : public std::enable_shared_from_this<TcpClient>
	{
	public:
		TcpClient(size_t id, ip::tcp::socket socket, 
			std::shared_ptr<TcpServer> server, 
			std::function<void(std::unique_ptr<Message>)> messageReceived) :
			mId(id),
			mSocket(std::move(socket)),
			mMessageReceived(messageReceived),
			mServer(server)
		{
		}
		void close();
		void run();
		void sendMessgae(std::string& message);
	private:
		size_t mId;
		ip::tcp::socket mSocket;
		char buffer[1024];
		std::function<void(std::unique_ptr<Message>)> mMessageReceived;
		std::shared_ptr<TcpServer> mServer;
	};
};

