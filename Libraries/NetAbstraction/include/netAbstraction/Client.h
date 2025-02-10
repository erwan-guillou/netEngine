#pragma once

#include <vector>
#include <functional>

#include "netAbstraction/CallbackContainer.h"
#include "netAbstraction/NetProcess.h"

struct ClientCallbacks
{
    using ReceiverCallback = std::function<void(const std::vector<char>& data)>;
    using ConnectedCallback = std::function<void()>;
    using DisconnectedCallback = std::function<void()>;
    using HandshakingCallback = std::function<bool()>;
};

class Client
    : public CallbackContainer<ClientCallbacks>
    , public NetProcess
{
public:
    Client(const std::string& ip, unsigned short port);
    ~Client();
    //
    virtual bool connect() = 0;
    virtual void disconnect() = 0;
    //
    virtual bool start() = 0;
    virtual void stop() = 0;
    //
    virtual bool send(const std::vector<char>& data, bool fake = false) = 0;
    virtual bool receive(std::vector<char>& outData) = 0;
protected:
    Layer::Address serverAddr;

};
