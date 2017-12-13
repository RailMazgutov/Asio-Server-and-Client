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

namespace
{
	using namespace asio;

	template<typename Server, std::size_t bufferSize>
	class Client
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
					server_->clientDisconnected(id_);					
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
	 

	class TcpServer final : public BaseServer, public std::enable_shared_from_this<TcpServer>
	{
	public:
		using Client_t = Client<TcpServer, 1024 * 1024>;
		friend Client_t;
		TcpServer(): address(ip::address::from_string("127.0.0.1")),
		             endPoint(address, 8080),
		             acceptor(service, endPoint),
		             socket(service), lastClientId(0), messages_lock(std::make_shared<std::recursive_mutex>()),
					 clients_lock(std::make_shared<std::mutex>())
		{
		}

		TcpServer(std::function<void(std::shared_ptr<Message>)> callback) : running(false),
			messageReceivedCallback(callback),
			address(ip::address::from_string("127.0.0.1")),
			endPoint(address, 8080), acceptor(service, endPoint),
			socket(service), lastClientId(0), messages_lock(std::make_shared<std::recursive_mutex>()),
			clients_lock(std::make_shared<std::mutex>())
		{			
		}

		void run() override
		{
			if (!running) {
				running = true;
				acceptClient();
				{
					std::lock_guard<std::recursive_mutex> section(*messages_lock);
					for (int i = 0; i < MAX_THREADS_COUNT; i++)
					{
						messageHandlers.emplace_back(std::make_shared<std::thread>(
							[&running = running, &messages = messages, lock = messages_lock](auto& callback) mutable
						{
							while (running) {
								std::shared_ptr<Message> message;
								{
									std::lock_guard<std::recursive_mutex> section(*lock);
									if (!running)
										break;
									if (messages.size() > 0) {
										message = messages.front();
										messages.pop();
									}
								}
								if (message)
								{
									if (callback)
									{
										callback(message);
									}
									else
									{
										std::cout << "no callback" << std::endl;
									}
								}
								std::this_thread::sleep_for(std::chrono::milliseconds(100));
							}
						}, messageReceivedCallback)
						);
					}
				}
				service.run();
				
			}
		}
		void stop() override
		{
			if(running)
			{
				running = false;
				for (auto client : clients)
					client.second->close();

				for (auto thread : messageHandlers) {
					thread->join();
				}
				messageHandlers.clear();
				acceptor.close();
				service.stop();
			}
			
		}

		void sendMessage(Message& message) override
		{
			if (clients.find(message.sender) != clients.end())
				clients[message.sender]->sendMessgae(message.message);
		}
		void sendMessageToAll(std::string& message) override
		{
			std::cout << "TcpServer:sendMessage" << std::endl;
		}
		size_t connectionsCount() override
		{
			return clients.size();
		}
		
		~TcpServer()
		{
			std::for_each(messageHandlers.begin(), messageHandlers.end(), [](auto handler)mutable
			{
				handler->detach();
			});
			messageHandlers.clear();
		}
	private:
		void acceptClient()
		{
			acceptor.async_accept(socket,
				[this](std::error_code ec)
			{
				std::cout << "client" << std::endl;
				if (!ec)
				{
					
					auto client = std::make_shared<Client_t>(lastClientId, std::move(socket), shared_from_this());
					client->run();
					{
						std::unique_lock<std::mutex> critical_section(*clients_lock);
						clients.insert(std::make_pair(lastClientId, client));
						lastClientId++;
					}
					if(clientConnectedCallback)
					{
						clientConnectedCallback(lastClientId - 1);
					}
				}
				acceptClient();
			});
		}

		void messageReceived(std::shared_ptr<Message> message)
		{
			std::lock_guard<std::recursive_mutex>lock(*messages_lock);
			messages.push(message);
		}

		void clientDisconnected(std::size_t id)
		{
			if (clientDisconnectedCallback)
				clientDisconnectedCallback(id);

			std::unique_lock<std::mutex> lock(*clients_lock);
			if (clients.find(id) != clients.end())
				clients.erase(id);			
		}

		std::atomic<bool> running;

		//Callbacks for events
		std::function<void(std::shared_ptr<Message>)> messageReceivedCallback;
		std::function<void(std::size_t)> clientConnectedCallback;
		std::function<void(std::size_t)> clientDisconnectedCallback;

		//Asio for tcp
		io_service service;
		ip::address address;
		ip::tcp::endpoint endPoint;
		ip::tcp::acceptor acceptor;
		ip::tcp::socket socket;

		size_t lastClientId;

		std::shared_ptr<std::recursive_mutex> messages_lock;
		std::shared_ptr<std::mutex> clients_lock;

		std::queue<std::shared_ptr<Message>> messages;
		std::vector<std::shared_ptr<std::thread>> messageHandlers;
		std::map<size_t, std::shared_ptr<Client_t>> clients;
	};
}

//template<typename MessageReceived, typename ClientConnected, typename ClientDisconnected>
//std::shared_ptr<BaseServer> getServer(ProtocolType type, MessageReceived msgHandler, ClientConnected connectionHandler, ClientDisconnected diconnectionHandler)
std::shared_ptr<BaseServer> getServer(ProtocolType type, std::function<void(std::shared_ptr<Message>)> callback)
{
	return std::make_shared<TcpServer>(callback);
}
