#include "netAbstraction/Client.h"

Client::Client(const std::string& ip, unsigned short port)
	: NetProcess("")
{
	serverAddr.ip = ip;
	serverAddr.port = port;
}

Client::~Client()
{
}
