// Server.cpp : Defines the exported functions for the DLL application.
//
#include "Server.h"
#include <asio.hpp>

Server::Message::Message() :
	mAuthorId(0),
	mMessage("")
{

}

Server::Message::Message(size_t authorId, const std::string& message) :
	mAuthorId(authorId),
	mMessage(message)
{
}

size_t Server::Message::getAuthorId() const
{
	return mAuthorId;
}

std::string& Server::Message::getMessage()
{
	return mMessage;
}
