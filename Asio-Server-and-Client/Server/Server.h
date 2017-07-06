#pragma once

#include <memory>
#include <string>
#include <functional>

class Server
{
public:
	static const std::string DEFAULT_LOCAL_IP; // "0.0.0.0"
	static const unsigned int DEFAULT_PORT = 8800;

	class Message
	{
	public:
		Message();
		Message(size_t authorId, const std::string& message);

		size_t getAuthorId() const;
		std::string& getMessage();
	private:
		size_t mAuthorId;
		std::string mMessage;
	};

	virtual ~Server() = 0;

	//Main Interface
	void virtual run() = 0;
	void virtual reuse_addr() = 0;
	void virtual stop() = 0;
	void virtual sendToClient(Message& message) = 0;

	//Register Callbacks
	virtual void registerMessageReceiveCallback(std::function<void(Message* message)>) = 0;
	virtual void registerMessageClientConnected(std::function<void(size_t client_id)>) = 0;
	virtual void registerMessageClientDisconnected(std::function<void(size_t client_id)>) = 0;

	//Inner Callbacks
	virtual void client_connection_handler() = 0;
	virtual void client_disconnection_handler(size_t client_id) = 0;
};
