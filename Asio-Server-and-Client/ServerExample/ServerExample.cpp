// ServerExample.cpp : Defines the entry point for the console application.
//
#include <BaseServer.h>
#include "stdafx.h"


int main()
{
	auto server = getServer(ProtocolType::TCP, 0, 0, 0);
    return 0;
}

