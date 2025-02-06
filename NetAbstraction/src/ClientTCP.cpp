#include "netAbstraction/ClientTCP.h"

#include <thread>
#include <iostream>

ClientTCP::ClientTCP(const std::string& ip, int port)
    : Client(ip, port)
{
}

ClientTCP::~ClientTCP() {
    stop();
}

bool ClientTCP::start() {
    if (startedFlag.load()) return false;

    // Start listen thread if needed
    stopFlag.store(false);
    std::thread(&ClientTCP::_listenForMessages, this).detach();
    startedFlag.store(true);

    return true;
}

void ClientTCP::stop() {
    if (!startedFlag.load()) return;
    stopFlag.store(true);
    while (threadsCompleted.load() >  0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));  // Poll every 100ms
    }
    startedFlag.store(false);
}

bool ClientTCP::connect()
{
    if (isConnected.load()) return false;
    {
        if (!LayerTCP::connectTo(sock, serverAddr))
        {
            std::cerr << "Error connecting to server: " << WSAGetLastError() << std::endl;
            return false;
        }
    }
    isConnected.store(true);
    bool isGood = true;
    if (_cbHandshake)
    {
        isGood = _cbHandshake();
    }
    if (isGood)
    {
        for (auto& callback : _cbConnect)
            callback.second();
    }
    else
    {
        std::cerr << "handshake error" << std::endl;
        LayerTCP::closeSocket(sock);
        isConnected.store(false);
        return false;
    }
    return true;
}

void ClientTCP::disconnect()
{
    if (!isConnected.load()) return;
    for (auto& callback : _cbDisconnect)
        callback.second();
    LayerTCP::closeSocket(sock);
    isConnected.store(false);
}

bool ClientTCP::send(const std::vector<char>& data, bool fake) {
    if (!isConnected.load()) return false;
    Layer::Socket _sock;
    {
        _sock = sock;
    }
    if (LayerTCP::send(_sock, data, serverAddr)) return true;
    {
        disconnect();
    }
    return false;
}

bool ClientTCP::receive(std::vector<char>& outData) {
    if (!isConnected.load()) return false;
    Layer::Address cli;
    Layer::Socket _sock;
    {
        _sock = sock;
    }
    if (LayerTCP::receive(_sock,outData,cli)) return true;
    {
        disconnect();
    }
    return false;
}

void ClientTCP::_listenForMessages()
{
    threadsCompleted.fetch_add(1, std::memory_order_release);
    while (!stopFlag.load()) {
        if (!isConnected.load()) continue;
        bool isGood = true;
        {
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
            if (result > 0 && readSet)
            {
                std::vector<char> buffer;
                bool res;
                res = LayerTCP::partial_receive(sock, buffer, serverAddr);
                if (res)
                {
                    if (buffer.size() > 0)
                    {
                        for (auto& callback : _cbReceive)
                            callback.second(buffer);
                    }
                }
                else
                {
                    std::cerr << "receive error" << std::endl;
                    isGood = false;
                }
            }
        }
        if (!isGood) disconnect();
    }
    threadsCompleted.fetch_sub(1, std::memory_order_release);
}
