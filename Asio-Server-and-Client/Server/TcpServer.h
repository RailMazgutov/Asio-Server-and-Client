#pragma once
#include "Server.h"
#include <asio/ip/tcp.hpp>
#include <asio/io_service.hpp>
#include <mutex>
#include <map>
#include <queue>

using namespace asio;
class TcpServer : public Server
{	
public:
	//Main interface
	TcpServer();
	TcpServer(const std::string& address, short port);

	~TcpServer();

	void run() override;

	void reuse_addr() override;

	void stop() override;

	void sendToClient(Message& message) override;

	//Register Callbacks
	virtual void registerMessageReceiveCallback(std::function<void(Message* message)>);
	virtual void registerMessageClientConnected(std::function<void(size_t client_id)>);
	virtual void registerMessageClientDisconnected(std::function<void(size_t client_id)>);

private:
	class Client;
	//PRIVATE CONSTANTS (или почти константы)
	size_t MAX_THREADS_COUNT;
	bool IS_RUNNING;

	//SERVICE
	std::mutex mLock;
	std::queue<Message *> mMessages;
	std::vector<std::thread*> mMessageHandlers;
	std::map<size_t, std::shared_ptr<Client>> mClients;
	size_t mLastClientId;
	
	void client_connection_handler();
	void client_disconnection_handler(size_t client_id);

	std::function<void(size_t client_id)> client_connected_callback;
	std::function<void(Message* message)> message_received_callback;
	std::function<void(size_t client_id)> client_disconnected_callback;

	//ASIO
	io_service mService;
	ip::address mIpAddress;
	ip::tcp::endpoint mEndPoint;
	ip::tcp::acceptor mAcceptor;
	ip::tcp::socket mSocket;

public:

	class Client : public std::enable_shared_from_this<Client>
	{
	public:
		Client(int id, asio::ip::tcp::socket socket, Server* server, std::function<void(Server::Message*)> messageReceivedCallback) :
			_id(id),
			_socket(std::move(socket)),
			_message_received_callback(messageReceivedCallback),
			_server(server)
		{
		}
		void close();
		void run();
		void sendMessgae(std::string& message);
	private:

		size_t _id;
		
		char _buffer[1024];
		std::function<void(Server::Message*)> _message_received_callback;

		Server* _server;
		ip::tcp::socket _socket;
	};
};

