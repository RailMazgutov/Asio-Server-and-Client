#pragma once
#include <memory>
#include <string>
#include <functional>

#ifdef SERVER_EXPORTS
#define SERVER_API __declspec(dllexport) 
#else
#define SERVER_API __declspec(dllimport) 
#endif

struct Message
{
	Message() = default;
	Message(size_t id, std::string msg): sender(id), message(std::move(msg)) {
		
	}

	size_t sender;
	std::string message;
};

enum class ProtocolType
{
	TCP,
	UDP
};
enum class BufferSize
{
	B128 = 128,
	B256 = 256,
	B512 = 512,
	B1024 = 1024,
	KB128 = 1024 * 128,
	KB256 = 1024 * 256,
	KB512 = 1024 * 512,
	KB1024 = 1024 * 1024,
	MB10 = 1024 * 1024 * 10,
	MB100 = 1024 * 1024 * 100
};

SERVER_API class BaseServer
{
public:
	static constexpr unsigned int MAX_THREADS_COUNT = 4;
	static const std::string DEFAULT_LOCAL_IP;	// 127.0.0.1
	static constexpr unsigned int DEFAULT_PORT = 8080;		// 8080

	SERVER_API virtual ~BaseServer() = default;

	//virtual void registerMessageReceivedCallback(std::function<void(std::shared_ptr<Message> message)>) = 0;
	//virtual void registerClientConnectedCallback(std::function<void(size_t client_id)>) = 0;
	//virtual void registerClientDisconnectedCallback(std::function<void(size_t client_id)>) = 0;

	SERVER_API virtual void run() = 0;
	SERVER_API virtual void stop() = 0;

	SERVER_API virtual void sendMessage(Message& message) = 0;
	SERVER_API virtual void sendMessageToAll(std::string& message) = 0;
	SERVER_API virtual size_t connectionsCount() = 0;
};

//template<typename MessageReceived, typename ClientConnected, typename ClientDisconnected>
SERVER_API std::shared_ptr<BaseServer> getServer(std::string ip, unsigned port, ProtocolType type = ProtocolType::TCP, BufferSize size = BufferSize::B1024, 
	bool useBlob = false,
	std::function<void(std::shared_ptr<Message>)> messageReceivedCallback = {},
	std::function<void(std::size_t)> clientConnectedCallback = {},
	std::function<void(std::size_t)> clientDisconnectedCallback = {});
