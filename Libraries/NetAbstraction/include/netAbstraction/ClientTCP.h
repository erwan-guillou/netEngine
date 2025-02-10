#pragma once

#include <vector>
#include <string>

#include "netAbstraction/Client.h"

class ClientTCP
    : public Client
{
public:
    ClientTCP(const std::string& ip, int port);
    ~ClientTCP();

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
