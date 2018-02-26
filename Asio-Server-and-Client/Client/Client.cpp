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
			service_.run();
			connected_ = true;
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
		char buff[BufferSize];
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
	bool connected_ { false };
	asio::io_service service_;
	unsigned int port_;
	asio::ip::address ipAddress_;
	asio::ip::tcp::socket socket_;
	asio::ip::tcp::endpoint endPoint_;
};

CLIENT_API std::shared_ptr<Client> getClient(std::string ip, unsigned port, ProtocolType type, BufferSize size, bool useBlob)
{
	return std::make_shared<TcpClient<1024>>(ip, port);
}
