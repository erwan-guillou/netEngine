#pragma once

#include <vector>
#include <map>

#include "netAbstraction/Server.h"

class ServerUDP : virtual public Server
{
public:
    ServerUDP(const int basePort, const std::string& intf = "");
    virtual ~ServerUDP();
    virtual bool connect();
    virtual void disconnect();
    virtual bool start();
    virtual void stop();
    virtual void send(const std::vector<char>& data);
    virtual void send(const Layer::Address& clientKey, const std::vector<char>& data);
    virtual void receive(const Layer::Address& client, std::vector<char>& data);
protected:
    std::vector<Layer::Address> clients;
    Layer::Socket _unicastSock;
    Layer::Address _broadcast;
    void _listenForMessages();
};
