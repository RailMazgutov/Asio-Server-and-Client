#include "TcpServer.h"

void TcpServer::Client::close()
{
	try {
		_socket.shutdown(asio::ip::tcp::socket::shutdown_both);
		_socket.close();
	}
	catch (error_code error)
	{

	}
	_server->client_disconnection_handler(_id);
}

void TcpServer::Client::run()
{
	auto self(shared_from_this());
	_socket.async_read_some(asio::buffer(_buffer, 1024),
		[this, self](std::error_code error_code, std::size_t length)
	{
		if (!error_code)
		{
			std::string message_buffer(_buffer, length);
			Message* message = new Message(_id, message_buffer);
			_message_received_callback(message);
			run();
		}
		else if (error::eof == error_code || error::connection_reset == error_code)
		{
			close();
		}
	});
}

void TcpServer::Client::sendMessgae(std::string& message)
{
	_socket.write_some(asio::buffer(message));
}


TcpServer::TcpServer():Server(),
	mIpAddress(ip::address::from_string(DEFAULT_LOCAL_IP)),
	mEndPoint(mIpAddress, DEFAULT_PORT),
	mAcceptor(mService, mEndPoint),
	mSocket(mService)
{
	mLastClientId = 0;
	IS_RUNNING = false;
	MAX_THREADS_COUNT = 4;
}

TcpServer::TcpServer(const std::string& address, short port) :Server(),
	mIpAddress(ip::address::from_string(address)),
	mEndPoint(mIpAddress, port),
	mAcceptor(mService, mEndPoint),
	mSocket(mService)
{
	mLastClientId = 0;
	IS_RUNNING = false;
	MAX_THREADS_COUNT = 4;
}

TcpServer::~TcpServer()
{
}

void TcpServer::run()
{
	mLock.lock();

	if (!IS_RUNNING) {
		IS_RUNNING = true;
		client_connection_handler();
		for (int i = 0; i < MAX_THREADS_COUNT; i++)
		{
			std::thread* handler = new std::thread([this]()mutable
			{
				while (IS_RUNNING) {
					if (mLock.try_lock())
					{
						if (!IS_RUNNING)
							break;
						if (mMessages.size() > 0) {
							Message* message = mMessages.front();
							mMessages.pop();
							mLock.unlock();
							if(message_received_callback)
								message_received_callback(message);
							delete message;
						}
						else
						{
							mLock.unlock();
							std::this_thread::sleep_for(chrono::milliseconds(100));
						}
					}
					else
					{
						std::this_thread::sleep_for(chrono::milliseconds(100));
					}
				}
			});
			mMessageHandlers.push_back(handler);
		}
		mLock.unlock();
		mService.run();
		return;
	}
	mLock.unlock();
}

void TcpServer::sendToClient(Message& message)
{
}





//Client Callbacks
void TcpServer::client_connection_handler()
{
}

void TcpServer::client_disconnection_handler(size_t client_id)
{
}