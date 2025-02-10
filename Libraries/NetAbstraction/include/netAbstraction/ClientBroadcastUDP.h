#pragma once

#include <vector>
#include <string>

#include "netAbstraction/Client.h"

class ClientBroadcastUDP
    : public Client
{
public:
    ClientBroadcastUDP(const std::string& ip, unsigned short port);
    ~ClientBroadcastUDP();

    bool start();
    void stop();

    bool connect();
    void disconnect();
    // Send data to the server
    bool send(const std::vector<char>& data, bool fake = false);
    bool receive(std::vector<char>& outData);
protected:

    void _listenForMessages();
};
