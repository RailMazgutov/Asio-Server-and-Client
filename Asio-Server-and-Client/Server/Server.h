#pragma once

#include <memory>
#include <string>
#include <functional>

class Server
{
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

	static const std::string DEFAULT_LOCAL_IP;
	static const unsigned int DEFAULT_PORT;

	Server(const std::string& address, int port);

	virtual void registerMessageReceiveCallback(std::function<void(Message* message)>) = 0;
	virtual void registerMessageClientAccepted(std::function<void(size_t client_id)>) = 0;
	virtual void registerMessageClientDisconnected(std::function<void(size_t client_id)>) = 0;

	virtual ~Server() = 0;

	void virtual run() = 0;

	void virtual reuse_addr() = 0;

	void virtual stop() = 0;

	void virtual sendToClient(Message& message) = 0;

	void virtual clientDisconnected(size_t client_id) = 0;
};
