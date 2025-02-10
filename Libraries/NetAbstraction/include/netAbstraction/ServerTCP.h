#pragma once

#include <map>
#include <vector>

#include "netAbstraction/Server.h"

class ServerTCP : virtual public Server
{
public:
    ServerTCP(const int basePort, const std::string& intf = "");
    virtual ~ServerTCP();
    virtual bool connect();
    virtual void disconnect();
    virtual bool start();
    virtual void stop();
    virtual void send(const std::vector<char>& data);
    virtual void send(const Layer::Address& client, const std::vector<char>& data);
    virtual void receive(const Layer::Address& client, std::vector<char>& data);
protected:
    std::map < Layer::Address, Layer::Socket> _toSocket;
    std::map < Layer::Socket, Layer::Address > _fromSocket;
    std::vector<Layer::Socket> _listenTo;

    // thread functions
    void _acceptClients();
    void _listenForMessages();
};
