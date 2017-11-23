#pragma once
#include <memory>
#include <string>
#include <functional>

class Message
{
public:
	Message() = default;

	Message(const Message& message) = default;
	Message& operator=(const Message& message) = default;

	Message(Message&& message) = default;
	Message& operator=(Message&& message) = default;

	Message(size_t sender, const std::string& message): 
		sender_(sender), 
		message_(message) {}

	Message(size_t sender, std::string&& message) :
		sender_(sender),
		message_(std::move(message)) {}

	~Message() = default;

	size_t sender() const
	{
		return sender_;
	}

	std::string& message()
	{
		return message_;
	}
private:
	size_t sender_;
	std::string message_;
};

enum class ProtocolType
{
	TCP,
	UDP
};

class BaseServer
{
public:
	BaseServer();
	virtual ~BaseServer() = default;

	virtual void registerMessageReceiveCallback(std::function<void(std::shared_ptr<Message> message)>) = 0;
	virtual void registerMessageClientConnected(std::function<void(size_t client_id)>) = 0;
	virtual void registerMessageClientDisconnected(std::function<void(size_t client_id)>) = 0;

	virtual void run() = 0;
	virtual void stop() = 0;

	virtual void sendMessage(Message& message) = 0;
	virtual void sendMessageToAll(Message& message) = 0;
	virtual size_t connectionsCount() = 0;
};
