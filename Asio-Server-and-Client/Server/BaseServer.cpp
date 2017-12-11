#include "BaseServer.h"

#include <iostream>

#include <atomic>
#include <queue>
#include <mutex>
#include <memory>
#include <map>

#include <asio.hpp>
#include <asio/io_service.hpp>
#include <asio/ip/address.hpp>
#include <asio/ip/tcp.hpp>

const std::string BaseServer::DEFAULT_LOCAL_IP	= "127.0.0.1";
const unsigned int BaseServer::DEFAULT_PORT		= 8080;

namespace
{
	using namespace asio;

	template<typename Server, std::size_t bufferSize>
	class Client : public std::enable_shared_from_this<Client<Server, bufferSize>>
	{
	public:
		Client(int id, ip::tcp::socket socket, std::shared_ptr<Server> server) :
			id_(id),
			socket_(std::move(socket)),
			server_(server)
		{
		}
		void close()
		{
			try {
				socket_.shutdown(asio::ip::tcp::socket::shutdown_both);
				socket_.close();
			}
			catch (asio::error_code error)
			{

			}

			server_->clientDisconnected(id_);
		}
		void run()
		{
			socket_.async_read_some(asio::buffer(buffer, bufferSize),
				[this](std::error_code ec, std::size_t length)
			{
				if (!ec)
				{
					std::string message_buffer(buffer, length);
					std::shared_ptr<Message> message = std::make_shared<Message>(id_, message_buffer);
					server_->messageReceived(message);
					run();
				}
				else if ((asio::error::eof == ec) ||
					(asio::error::connection_reset == ec))
				{
					close();
				}
			});
		}
		void sendMessgae(std::string& message)
		{
			socket_.write_some(asio::buffer(message));
		}
	private:
		std::size_t id_;
		ip::tcp::socket socket_;
		char buffer[bufferSize];
		std::shared_ptr<Server> server_;
	};
	 

	template<typename MessageReceived, typename ClientConnected = void, typename ClientDisconnected = void>
	class TcpServer final : public BaseServer
	{
		using Server = TcpServer<MessageReceived, ClientConnected, ClientDisconnected>;
		using Client_t = Client<Server, 1024 * 1024>;
		friend Client_t;
		TcpServer();
		void run() override
		{
			std::cout << "TcpServer:run" << std::endl;
		}
		void stop() override
		{
			std::cout << "TcpServer:stop" << std::endl;
		}

		void sendMessage(Message& message) override
		{
			std::cout << "TcpServer:sendMessage" << std::endl;
		}
		void sendMessageToAll(std::string& message) override
		{
			std::cout << "TcpServer:sendMessage" << std::endl;
		}
		size_t connectionsCount() override
		{
			std::cout << "TcpServer:connectionsCount" << std::endl;
			return 0;
		}
		
	private:
		

		void messageReceived(std::shared_ptr<Message> message)
		{

		}

		void clientDisconnected(std::size_t id)
		{

		}

		std::atomic<bool> running;

		//Callbacks for events
		MessageReceived messageReceivedCallback;
		ClientConnected clientConnectedCallback;
		ClientDisconnected clientDisconnectedCallback;

		//Asio for tcp
		io_service service;
		ip::address address;
		ip::tcp::endpoint endPoint;
		ip::tcp::acceptor acceptor;
		ip::tcp::socket socket;

		size_t lastClientId;

		std::mutex lock;
		std::queue<std::shared_ptr<Message>> messages;
		std::vector<std::shared_ptr<std::thread>> messageHandlers;
		std::map<size_t, std::shared_ptr<Client_t>> clients;
	};
}

//template<typename MessageReceived, typename ClientConnected, typename ClientDisconnected>
//std::shared_ptr<BaseServer> getServer(ProtocolType type, MessageReceived msgHandler, ClientConnected connectionHandler, ClientDisconnected diconnectionHandler)
std::shared_ptr<BaseServer> getServer(ProtocolType type, int msgHandler, int connectionHandler, int diconnectionHandler)
{
	return std::make_shared<TcpServer<int, int, int>>();
}
