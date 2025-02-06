#pragma once

#include <vector>
#include <string>

#include "netAbstraction/Client.h"

class ClientUDP
    : public Client
{
public:
    ClientUDP(const std::string& ip, unsigned short port);
    ~ClientUDP();

    bool start();
    void stop();

    bool connect();
    void disconnect();
    // Send data to the server
    bool send(const std::vector<char>& data, bool fake = false);
    bool receive(std::vector<char>& outData);
protected:
    Layer::Socket _unicastSock;

    void _listenForMessages();
};
