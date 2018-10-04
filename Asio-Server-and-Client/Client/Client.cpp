// Client.cpp : Defines the exported functions for the DLL application.
//

#include <asio.hpp>
#include <asio/io_service.hpp>
#include <asio/ip/address.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/impl/src.hpp>
#include "Client.h"

template <std::size_t BufferSize>
class TcpClient : public Client
{
public:
	TcpClient(const std::string& address, unsigned int port)
		:port_(port), socket_(service_)
	{
		ipAddress_ = asio::ip::address::from_string(address);
		endPoint_.address(ipAddress_);
		endPoint_.port(port_);
	}

	void connect() override
	{
		if (connected_)
			return;

		asio::error_code error;
		socket_.connect(endPoint_, error);
		if(!error)
		{
			connected_ = true;
			service_.run();			
		}
	}

	bool isConnected() override
	{
		return connected_;
	}

	void disconnect() override
	{
		if (connected_)
		{
			asio::error_code error;
			socket_.shutdown(asio::socket_base::shutdown_both, error);
			socket_.close(error);
			service_.stop();
			connected_ = false;
		}
	}
	size_t send(const std::string& data) override
	{
		if (connected_) {
			asio::error_code err_code;
			auto size = socket_.write_some(asio::buffer(data, data.size()), err_code);
			if (size == 0)
			{
				connected_ = false;				
			}
			return size;
		}
		return 0;
	}
	std::string read() override
	{
		
		asio::error_code err_code;
		size_t bytes_received = socket_.read_some(asio::buffer(buff), err_code);
		if ((asio::error::eof == err_code) || (asio::error::connection_reset == err_code))
		{
			connected_ = false;
			return std::string();
		}
			

		std::string msg(buff, bytes_received);
		return msg;
	}
	~TcpClient() override = default;
	
private:
	char buff[BufferSize];
	bool connected_ { false };
	asio::io_service service_;
	unsigned int port_;
	asio::ip::address ipAddress_;
	asio::ip::tcp::socket socket_;
	asio::ip::tcp::endpoint endPoint_;
};

template <std::size_t BufferSize>
class BlobTcpClient : public TcpClient<BufferSize>
{
public:
	BlobTcpClient(const std::string& address, unsigned int port): TcpClient<BufferSize>(address, port)
	{
	}
	size_t send(const std::string& data) override
	{
		if (TcpClient<BufferSize>::isConnected()) {
			int sizeNumber = data.size();
			std::string sizeBytes(reinterpret_cast<char*>(&sizeNumber), sizeof sizeNumber);
			TcpClient<BufferSize>::send(sizeBytes);
			return TcpClient<BufferSize>::send(data);
		}
		return 0;
	}
	std::string read() override
	{
		int blobSize = 0;
		if (buffer.size() > sizeof(int))
		{
			blobSize = *reinterpret_cast<int*>(&buffer[0]);
			buffer = buffer.substr(sizeof blobSize);
		}
		else
		{
			while (buffer.size() < sizeof blobSize)
			{
				auto msg = TcpClient<BufferSize>::read();
				if (msg.empty())
					return "";

				buffer += msg;
			}
			blobSize = *reinterpret_cast<int*>(&buffer[0]);
			buffer = buffer.substr(sizeof blobSize);
		}
		while (buffer.size() < blobSize)
		{
			auto msg = TcpClient<BufferSize>::read();
			if (msg.empty())
				return "";

			buffer += msg;
		}
		auto msg = buffer.substr(0, blobSize);
		buffer = buffer.substr(blobSize);
		return msg;
	}

private:
	std::string buffer;
};

CLIENT_API std::shared_ptr<Client> getClient(std::string ip, unsigned port, ProtocolType type, const BufferSize size, bool useBlob)
{
	//const auto s = static_cast<std::size_t>(size);
	if (useBlob)
		return std::make_shared<BlobTcpClient<1024*1024>>(ip, port);

	return std::make_shared<TcpClient<1024 * 1024>>(ip, port);
}
