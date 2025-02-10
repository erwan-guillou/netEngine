#include "netAbstraction/ServerBroadcastUDP.h"

#include <thread>
#include <iostream>

ServerBroadcastUDP::ServerBroadcastUDP(const int basePort, const std::string& intf)
    : Server(basePort, intf)
{
}

ServerBroadcastUDP::~ServerBroadcastUDP() {
}

bool ServerBroadcastUDP::start() {
    if (startedFlag.load()) return false;
    stopFlag.store(false);
    startedFlag.store(true);
    return true;
}

void ServerBroadcastUDP::stop()
{
    if (!startedFlag.load()) return;
    stopFlag.store(true);
    while (threadsCompleted.load() > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    startedFlag.store(false);
}

bool ServerBroadcastUDP::connect()
{
    if (isConnected.load()) return false;

    if (!LayerUDP::openBroadcastSocket(_port, sock, intfToUse))
    {
        std::cout << "error opening broadcast socket" << std::endl;
        return false;
    }
    _broadcast.ip = GetBroadcastAddress(intfToUse);
    _broadcast.port = _port;
    isConnected.store(true);
    return true;
}

void ServerBroadcastUDP::disconnect()
{
    if (!isConnected.load()) return;
    {
        std::lock_guard<std::mutex> lock(clientMutex);
        LayerUDP::closeSocket(sock);
    }
    isConnected.store(false);
}

void ServerBroadcastUDP::send(const std::vector<char>& data)
{
    // broadcast to all clients
    LayerUDP::send(sock, data, _broadcast);
}

void ServerBroadcastUDP::send(const Layer::Address& client, const std::vector<char>& data)
{
}

void ServerBroadcastUDP::receive(const Layer::Address& client, std::vector<char>& data)
{
}
