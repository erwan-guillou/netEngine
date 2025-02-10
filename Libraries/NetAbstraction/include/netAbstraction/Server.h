#pragma once

#include <vector>
#include <functional>

#include "netAbstraction/CallbackContainer.h"
#include "netAbstraction/NetProcess.h"

struct ServerCallbacks
{
    using ReceiverCallback = std::function<void(const Layer::Address& from, const std::vector<char>& data)>;
    using ConnectedCallback = std::function<void(const Layer::Address& from)>;
    using DisconnectedCallback = std::function<void(const Layer::Address& from)>;
    using HandshakingCallback = std::function<bool(const Layer::Address& from, const std::vector<char>& message)>;
};

class Server
    : public CallbackContainer<ServerCallbacks>
    , public NetProcess
{
public:
    Server(const int basePort, const std::string& intf = "");
    virtual ~Server();
    //
    virtual bool connect() = 0;
    virtual void disconnect() = 0;
    //
    virtual bool start() = 0;
    virtual void stop() = 0;
    //
    virtual void send(const std::vector<char>& data) = 0;
    virtual void send(const Layer::Address& clientSocket, const std::vector<char>& data) = 0;
    virtual void receive(const Layer::Address& client, std::vector<char>& data) = 0;
protected:
    int _port;
};
