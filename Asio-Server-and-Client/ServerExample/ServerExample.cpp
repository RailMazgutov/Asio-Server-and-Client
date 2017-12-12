// ServerExample.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include <BaseServer.h>
#include <iostream>


int main()
{
	auto server = getServer(ProtocolType::TCP, [](auto message)
	{
		std::cout << "Message Received" << std::endl;
	});
	server->connectionsCount();
	server->run();
	//server->sendMessage(Message(0, "message"));
	std::string msg = "Message";
	server->sendMessageToAll(msg);
    return 0;
}

