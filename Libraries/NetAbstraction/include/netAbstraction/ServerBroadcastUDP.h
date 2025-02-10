#pragma once

#include <vector>
#include <map>

#include "netAbstraction/Server.h"

class ServerBroadcastUDP : virtual public Server
{
public:
    ServerBroadcastUDP(const int basePort, const std::string& intf = "");
    virtual ~ServerBroadcastUDP();
    virtual bool connect();
    virtual void disconnect();
    virtual bool start();
    virtual void stop();
    virtual void send(const std::vector<char>& data);
    virtual void send(const Layer::Address& clientKey, const std::vector<char>& data);
    virtual void receive(const Layer::Address& client, std::vector<char>& data);
protected:
    Layer::Address _broadcast;
};
