// ServerExample.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include <BaseServer.h>
#include <iostream>
#include <chrono>
#include <thread>


int main()
{
	auto server = getServer(ProtocolType::TCP, [](auto message)
	{
		std::cout << message->message << std::endl;
	});
	server->connectionsCount();
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

