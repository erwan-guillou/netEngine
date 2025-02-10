#include "netAbstraction/ClientUDP.h"

#include <thread>
#include <iostream>

ClientUDP::ClientUDP(const std::string& ip, unsigned short port)
    : Client(ip, port)
{
}

ClientUDP::~ClientUDP() {
    stop();
}

bool ClientUDP::start() {
    if (startedFlag.load()) return false;

    // Start listen thread if needed
    stopFlag.store(false);
    std::thread(&ClientUDP::_listenForMessages, this).detach();
    startedFlag.store(true);

    return true;
}

void ClientUDP::stop() {
    if (!startedFlag.load()) return;
    stopFlag.store(true);
    while (threadsCompleted.load() > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));  // Poll every 100ms
    }
    startedFlag.store(false);
}

bool ClientUDP::connect()
{
    if (isConnected.load()) return false;

    std::lock_guard<std::mutex> lock(clientMutex);
    if (!LayerUDP::connectTo(_unicastSock, serverAddr))
    {
        std::cerr << "error opening unicast socket" << std::endl;
        return false;
    }
    isConnected.store(true);
    bool isGood = true;
    if (_cbHandshake)
    {
        isGood = _cbHandshake();
    }
    if (!isGood)
    {
        std::cerr << "handshake error" << std::endl;
        LayerUDP::closeSocket(_unicastSock);
        isConnected.store(false);
        return false;
    }
    if (!LayerUDP::openBroadcastSocket(serverAddr.port+10, sock))
    {
        LayerUDP::closeSocket(_unicastSock);
        isConnected.store(false);
        return false;
    }
    for (auto& callback : _cbConnect)
        callback.second();
    return true;
}

void ClientUDP::disconnect()
{
    if (!isConnected.load()) return;
    LayerUDP::closeSocket(_unicastSock);
    LayerUDP::closeSocket(sock);
    isConnected.store(false);
}

bool ClientUDP::send(const std::vector<char>& data, bool fake) {
    if (!isConnected.load()) return false;
    bool res = LayerUDP::send(_unicastSock, data, serverAddr, fake);
    return res;
}

bool ClientUDP::receive(std::vector<char>& outData) {
    Layer::Address addr;
    if (!isConnected.load()) return false;
    return LayerUDP::receive(_unicastSock, outData, addr);
}

void ClientUDP::_listenForMessages()
{
    threadsCompleted.fetch_add(1, std::memory_order_release);
    while (!stopFlag.load()) {
        if (!isConnected.load()) continue;
        bool isGood = true;
        {
            std::lock_guard<std::mutex> lock(clientMutex);
            bool readSet;
            int result;
            if (!isConnected.load()) continue;
            if (stopFlag.load()) break;
            result = net_select(sock, readSet);
            if (!isConnected.load()) continue;
            if (stopFlag.load()) break;

            if (result == SOCKET_ERROR)
            {
                std::cerr << "select() failed with error: " << WSAGetLastError() << std::endl;
                std::cerr << isConnected.load() << std::endl;
            }
            else if (result > 0)
            {
                if (readSet)
                {
                    std::vector<char> buffer;
                    bool res;
                    Layer::Address addr;
                    res = LayerUDP::partial_receive(sock, buffer, addr);
                    if (res && buffer.size() > 0)
                    {
                        for (auto& callback : _cbReceive)
                            callback.second(buffer);
                    }
                    else if (res)
                    {
                        // do nothing
                    }
                    else
                    {
                        std::cerr << "receive error" << std::endl;
                        isGood = false;
                    }
                }
                else
                {
                    std::cout << "activity on unknown socket" << std::endl;
                    isGood = false;
                }
            }
        }
        if (!isGood) disconnect();
    }
    threadsCompleted.fetch_sub(1, std::memory_order_release);
}
