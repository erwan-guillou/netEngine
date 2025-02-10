#include "netAbstraction/Server.h"

Server::Server(const int basePort, const std::string& intf)
	: NetProcess(intf)
    , _port(basePort)
{
}

Server::~Server()
{
}
