// ServerExample.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include <BaseServer.h>
#include <iostream>
#include <chrono>
#include <thread>


int main()
{
	std::shared_ptr<BaseServer> server;
	server = getServer("127.0.0.1" , 1339, ProtocolType::TCP, BufferSize::KB1024, true,[&](auto message)
	{
		std::cout << message->message << std::endl;
		std::cout << server->connectionsCount() << std::endl;
	});
	
	server->run();
	//server->sendMessage(Message(0, "message"));
	/*std::string msg = "Message";
	server->sendMessageToAll(msg);*/
	while(true)
	{
		std::this_thread::sleep_for(std::chrono::microseconds(10));
	}
    return 0;
}

