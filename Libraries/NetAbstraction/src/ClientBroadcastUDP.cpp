#include "netAbstraction/ClientBroadcastUDP.h"

#include <thread>
#include <iostream>

ClientBroadcastUDP::ClientBroadcastUDP(const std::string& ip, unsigned short port)
    : Client(ip, port)
{
}

ClientBroadcastUDP::~ClientBroadcastUDP() {
    stop();
}

bool ClientBroadcastUDP::start() {
    if (startedFlag.load()) return false;

    // Start listen thread if needed
    stopFlag.store(false);
    std::thread(&ClientBroadcastUDP::_listenForMessages, this).detach();
    startedFlag.store(true);

    return true;
}

void ClientBroadcastUDP::stop() {
    if (!startedFlag.load()) return;
    stopFlag.store(true);
    while (threadsCompleted.load() > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));  // Poll every 100ms
    }
    startedFlag.store(false);
}

bool ClientBroadcastUDP::connect()
{
    if (isConnected.load()) return false;

    std::lock_guard<std::mutex> lock(clientMutex);
    if (!LayerUDP::openBroadcastSocket(serverAddr.port, sock))
    {
        isConnected.store(false);
        return false;
    }
    for (auto& callback : _cbConnect)
        callback.second();
    return true;
}

void ClientBroadcastUDP::disconnect()
{
    if (!isConnected.load()) return;
    LayerUDP::closeSocket(sock);
    isConnected.store(false);
}

bool ClientBroadcastUDP::send(const std::vector<char>& data, bool fake) {
    if (!isConnected.load()) return false;
    return true;
}

bool ClientBroadcastUDP::receive(std::vector<char>& outData) {
    if (!isConnected.load()) return false;
    return true;
}

void ClientBroadcastUDP::_listenForMessages()
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
