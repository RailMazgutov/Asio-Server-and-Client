#pragma once
#include <string>
#include <memory>

#ifdef CLIENT_EXPORTS
#define CLIENT_API __declspec(dllexport) 
#else
#define CLIENT_API __declspec(dllimport) 
#endif

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

enum class ProtocolType
{
	TCP,
	UDP
};

CLIENT_API class Client
{
public:
	CLIENT_API virtual void connect() = 0;
	CLIENT_API virtual bool isConnected() = 0;
	CLIENT_API virtual	void disconnect() = 0;
	CLIENT_API virtual size_t send(const std::string& data) = 0;
	CLIENT_API virtual std::string read() = 0;

	CLIENT_API virtual ~Client() = default;
};

CLIENT_API std::shared_ptr<Client> getClient(std::string ip, unsigned port, ProtocolType type = ProtocolType::TCP, const BufferSize size = BufferSize::B1024,
	bool useBlob = false);
