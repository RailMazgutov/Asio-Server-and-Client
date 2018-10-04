

#include <atomic>
#include <queue>
#include <mutex>
#include <memory>
#include <map>
#include <iostream>
#include <iomanip>

#include <asio.hpp>
#include <asio/io_service.hpp>
#include <asio/ip/address.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/impl/src.hpp>
#include "BaseServer.h"

namespace NetworkServer {
	const std::string BaseServer::DEFAULT_LOCAL_IP = "127.0.0.1";
	const unsigned int BaseServer::MAX_THREADS_COUNT = std::thread::hardware_concurrency();

	namespace
	{
		using namespace asio;

		template <typename Client_t>
		class Server : public BaseServer
		{
			template<std::size_t bufferSize> friend class Client;
			template<std::size_t bufferSize> friend class BlobClient;
			friend Client_t;
		protected:
			virtual void messageReceived(std::shared_ptr<Message> message) = 0;
			virtual void clientDisconnected(std::size_t id) = 0;
		};

		template<std::size_t bufferSize>
		class Client
		{
		public:
			Client(int id, ip::tcp::socket socket, std::shared_ptr<Server<Client<bufferSize>>> server) :
				id_(id),
				socket_(std::move(socket)),
				server_(server)
			{
			}

			virtual ~Client() {}
			void close()
			{
				asio::error_code code;
				socket_.shutdown(asio::ip::tcp::socket::shutdown_both, code);
				socket_.close(code);
			}
			virtual void run()
			{
				socket_.async_read_some(asio::buffer(buffer_, bufferSize),
					[this](std::error_code ec, std::size_t length)
				{
					if (!ec)
					{
						std::string message_buffer(buffer_, length);
						std::shared_ptr<Message> message = std::make_shared<Message>(id_, std::move(message_buffer));
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

			virtual void sendMessgae(std::string& message)
			{
				socket_.write_some(asio::buffer(message));
			}
		protected:
			Client(int id, ip::tcp::socket socket) :
				id_(id),
				socket_(std::move(socket))
			{
			}

			std::size_t id_;
			ip::tcp::socket socket_;
			char buffer_[bufferSize];
			std::shared_ptr<Server<Client<bufferSize>>> server_;
		};

		template<std::size_t bufferSize>
		class BlobClient final
		{
		public:
			BlobClient(int id, ip::tcp::socket socket, std::shared_ptr<Server<BlobClient<bufferSize>>> server) :
				packStarted(false),
				blobSize(0),
				id_(id),
				socket_(std::move(socket)),
				server_(server)
			{
				messageHandling = [this](std::error_code ec, std::size_t length)
				{
					if (ec)
					{
						if ((asio::error::eof == ec) ||
							(asio::error::connection_reset == ec))
						{
							close();
							server_->clientDisconnected(id_);
						}
						return;
					}

					unsigned bufferPosition = 0;
					while (length) {
						if (!packStarted)
						{
							blobSize = *reinterpret_cast<unsigned*>(buffer_);
							packStarted = true;
							bufferPosition += sizeof(unsigned);
							length -= sizeof(unsigned);
							message_buffer.reserve(blobSize);
						}
						auto readBytes = std::min(length, blobSize);
						if (readBytes)
						{
							message_buffer.append(buffer_ + bufferPosition, readBytes);
							blobSize -= readBytes;
							bufferPosition += readBytes;
							length -= readBytes;
						}
						if (!blobSize)
						{
							packStarted = false;
							auto message = std::make_shared<Message>(id_, std::move(message_buffer));
							server_->messageReceived(message);
						}
					}
					run();
				};
			}
			void close()
			{
				asio::error_code code;
				socket_.shutdown(asio::ip::tcp::socket::shutdown_both, code);
				socket_.close(code);
			}
			void run()
			{
				socket_.async_read_some(asio::buffer(buffer_, bufferSize), messageHandling);
			}

			void sendMessgae(std::string& message)
			{
				int size = int(message.size());
				std::string bsize(reinterpret_cast<char*>(&size), sizeof(size));
				socket_.write_some(asio::buffer(bsize + message));
			}

		private:
			bool packStarted;
			std::size_t blobSize;
			std::string message_buffer;
			std::function<void(std::error_code ec, std::size_t length)> messageHandling;
			std::size_t id_;
			ip::tcp::socket socket_;
			char buffer_[bufferSize];
			std::shared_ptr<Server<BlobClient<bufferSize>>> server_;
		};

		template <typename Client_t>
		class TcpServer final : public Server<Client_t>, public std::enable_shared_from_this<TcpServer<Client_t>>
		{
		public:
			template<std::size_t bufferSize> friend class Client;
			template<std::size_t bufferSize> friend class BlobClient;
			friend Client_t;
			TcpServer() = delete;

			TcpServer(std::string ip = "127.0.0.1", unsigned port = 8080, std::function<void(std::shared_ptr<Message>)> messageReceived = {},
				std::function<void(std::size_t)> clientConnected = {},
				std::function<void(std::size_t)> clientDisconnected = {}) : running(false),
				messageReceivedCallback(messageReceived),
				clientConnectedCallback(clientConnected),
				clientDisconnectedCallback(clientDisconnected),
				address(ip::address::from_string(ip)),
				endPoint(address, port), acceptor(service, endPoint),
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
									}
									else
										std::this_thread::sleep_for(std::chrono::milliseconds(1));
								}
							}, messageReceivedCallback)
							);
						}
					}
					try {
						service.run();
					}
					catch (std::error_code& code) {}
					catch (...) {}
				}
			}
			void stop() override
			{
				if (running)
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
				for (auto client : clients)
				{
					client.second->sendMessgae(message);
				}
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
					if (!ec)
					{

						auto client = std::make_shared<Client_t>(lastClientId, std::move(socket), shared_from_this());
						client->run();
						{
							std::unique_lock<std::mutex> critical_section(*clients_lock);
							clients.insert(std::make_pair(lastClientId, client));
							lastClientId++;
						}
						if (clientConnectedCallback)
						{
							clientConnectedCallback(lastClientId - 1);
						}
					}
					acceptClient();
				});
			}

			void messageReceived(std::shared_ptr<Message> message) override
			{
				std::lock_guard<std::recursive_mutex>lock(*messages_lock);
				messages.push(message);
			}

			void clientDisconnected(std::size_t id) override
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

	std::shared_ptr<BaseServer> getServer(std::string ip, unsigned port, ProtocolType type, BufferSize size,
		bool useBlob,
		std::function<void(std::shared_ptr<Message>)> messageReceivedCallback,
		std::function<void(std::size_t)> clientConnectedCallback,
		std::function<void(std::size_t)> clientDisconnectedCallback)
	{
		switch (size)
		{
		case BufferSize::B128:
		{
			if (useBlob)
			{
				return std::make_shared<TcpServer<BlobClient<static_cast<std::size_t>(BufferSize::B128)>>>(ip, port, messageReceivedCallback, clientConnectedCallback, clientDisconnectedCallback);
			}
			return std::make_shared<TcpServer<Client<static_cast<std::size_t>(BufferSize::B128)>>>(ip, port, messageReceivedCallback, clientConnectedCallback, clientDisconnectedCallback);

		}
		case BufferSize::B256:
		{
			if (useBlob)
			{
				return std::make_shared<TcpServer<BlobClient<static_cast<std::size_t>(BufferSize::B256)>>>(ip, port, messageReceivedCallback, clientConnectedCallback, clientDisconnectedCallback);
			}
			return std::make_shared<TcpServer<Client<static_cast<std::size_t>(BufferSize::B256)>>>(ip, port, messageReceivedCallback, clientConnectedCallback, clientDisconnectedCallback);
		}
		case BufferSize::B512:
		{
			if (useBlob)
			{
				return std::make_shared<TcpServer<BlobClient<static_cast<std::size_t>(BufferSize::B512)>>>(ip, port, messageReceivedCallback, clientConnectedCallback, clientDisconnectedCallback);
			}
			return std::make_shared<TcpServer<Client<static_cast<std::size_t>(BufferSize::B512)>>>(ip, port, messageReceivedCallback, clientConnectedCallback, clientDisconnectedCallback);
		}
		case BufferSize::B1024:
		{
			if (useBlob)
			{
				return std::make_shared<TcpServer<BlobClient<static_cast<std::size_t>(BufferSize::B1024)>>>(ip, port, messageReceivedCallback, clientConnectedCallback, clientDisconnectedCallback);
			}
			return std::make_shared<TcpServer<Client<static_cast<std::size_t>(BufferSize::B1024)>>>(ip, port, messageReceivedCallback, clientConnectedCallback, clientDisconnectedCallback);
		}
		case BufferSize::KB128:
		{
			if (useBlob)
			{
				return std::make_shared<TcpServer<BlobClient<static_cast<std::size_t>(BufferSize::KB128)>>>(ip, port, messageReceivedCallback, clientConnectedCallback, clientDisconnectedCallback);
			}
			return std::make_shared<TcpServer<Client<static_cast<std::size_t>(BufferSize::KB128)>>>(ip, port, messageReceivedCallback, clientConnectedCallback, clientDisconnectedCallback);
		}
		case BufferSize::KB256:
		{
			if (useBlob)
			{
				return std::make_shared<TcpServer<BlobClient<static_cast<std::size_t>(BufferSize::KB256)>>>(ip, port, messageReceivedCallback, clientConnectedCallback, clientDisconnectedCallback);
			}
			return std::make_shared<TcpServer<Client<static_cast<std::size_t>(BufferSize::KB256)>>>(ip, port, messageReceivedCallback, clientConnectedCallback, clientDisconnectedCallback);
		}
		case BufferSize::KB512:
		{
			if (useBlob)
			{
				return std::make_shared<TcpServer<BlobClient<static_cast<std::size_t>(BufferSize::KB512)>>>(ip, port, messageReceivedCallback, clientConnectedCallback, clientDisconnectedCallback);
			}
			return std::make_shared<TcpServer<Client<static_cast<std::size_t>(BufferSize::KB512)>>>(ip, port, messageReceivedCallback, clientConnectedCallback, clientDisconnectedCallback);
		}
		case BufferSize::KB1024:
		{
			if (useBlob)
			{
				return std::make_shared<TcpServer<BlobClient<static_cast<std::size_t>(BufferSize::KB1024)>>>(ip, port, messageReceivedCallback, clientConnectedCallback, clientDisconnectedCallback);
			}
			return std::make_shared<TcpServer<Client<static_cast<std::size_t>(BufferSize::KB1024)>>>(ip, port, messageReceivedCallback, clientConnectedCallback, clientDisconnectedCallback);
		}
		case BufferSize::MB10:
		{
			if (useBlob)
			{
				return std::make_shared<TcpServer<BlobClient<static_cast<std::size_t>(BufferSize::MB10)>>>(ip, port, messageReceivedCallback, clientConnectedCallback, clientDisconnectedCallback);
			}
			return std::make_shared<TcpServer<Client<static_cast<std::size_t>(BufferSize::MB10)>>>(ip, port, messageReceivedCallback, clientConnectedCallback, clientDisconnectedCallback);
		}
		case BufferSize::MB100:
		{
			if (useBlob)
			{
				return std::make_shared<TcpServer<BlobClient<static_cast<std::size_t>(BufferSize::MB100)>>>(ip, port, messageReceivedCallback, clientConnectedCallback, clientDisconnectedCallback);
			}
			return std::make_shared<TcpServer<Client<static_cast<std::size_t>(BufferSize::MB100)>>>(ip, port, messageReceivedCallback, clientConnectedCallback, clientDisconnectedCallback);
		}
		}
		return std::shared_ptr<BaseServer>();
	}
}
