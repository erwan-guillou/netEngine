#include "netAbstraction/ServerUDP.h"

#include <thread>
#include <iostream>

ServerUDP::ServerUDP(const int basePort, const std::string& intf)
    : Server(basePort, intf)
{
}

ServerUDP::~ServerUDP() {
}

bool ServerUDP::start() {
    if (startedFlag.load()) return false;
    stopFlag.store(false);
    std::thread(&ServerUDP::_listenForMessages, this).detach();
    startedFlag.store(true);
    return true;
}

void ServerUDP::stop()
{
    if (!startedFlag.load()) return;
    stopFlag.store(true);
    while (threadsCompleted.load() > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    startedFlag.store(false);
}

bool ServerUDP::connect()
{
    if (isConnected.load()) return false;

    if (!LayerUDP::openBroadcastSocket(_port+10, sock, intfToUse))
    {
        std::cout << "error opening broadcast socket" << std::endl;
        return false;
    }
    if (!LayerUDP::openUnicastSocket(_port, _unicastSock, intfToUse))
    {
        std::cout << "error opening unicast socket" << std::endl;
        LayerUDP::closeSocket(sock);
        return false;
    }
    _broadcast.ip = GetBroadcastAddress(intfToUse);
    std::cout << _broadcast.ip << std::endl;
    _broadcast.port = _port+10;
    isConnected.store(true);
    return true;
}

void ServerUDP::disconnect()
{
    if (!isConnected.load()) return;
    {
        std::lock_guard<std::mutex> lock(clientMutex);
        LayerUDP::closeSocket(_unicastSock);
        LayerUDP::closeSocket(sock);
    }
    isConnected.store(false);
}

void ServerUDP::send(const std::vector<char>& data)
{
    // broadcast to all clients
    LayerUDP::send(sock, data, _broadcast);
}

void ServerUDP::send(const Layer::Address& client, const std::vector<char>& data)
{
    // unicast to one client
    LayerUDP::send(_unicastSock, data, client);
}

void ServerUDP::receive(const Layer::Address& client, std::vector<char>& data)
{
    // unicast receive from one client
}

void ServerUDP::_listenForMessages()
{
    // listen on unicast socket
    threadsCompleted.fetch_add(1, std::memory_order_release);
    while (!stopFlag.load()) {
        if (!isConnected.load()) continue;

        // Use select to implement a timeout on the accept call
        bool readfds = false;
        int activity = net_select(_unicastSock, readfds);

        if (activity > 0)
        {
            if (readfds) {
                std::vector<char> buffer;
                Layer::Address client;
                bool recv = LayerUDP::partial_receive(_unicastSock, buffer, client);

                if (recv && buffer.size() > 0) {
                    bool knownClient = false;
                    {
                        std::lock_guard<std::mutex> lock(clientMutex);
                        // test if client is in known client list
                        auto it = std::find(clients.begin(), clients.end(), client);
                        knownClient = it != clients.end();
                    }
                    if (!knownClient)
                    {
                        if (_cbHandshake)
                        {
                            if (_cbHandshake(client, buffer))
                            {
                                std::lock_guard<std::mutex> lock(clientMutex);
                                clients.push_back(client);
                                for (auto& callback : _cbConnect)
                                    callback.second(client);
                            }
                            else
                            {

                            }
                        }
                        else
                        {
                            std::lock_guard<std::mutex> lock(clientMutex);
                            clients.push_back(client);
                            for (auto& callback : _cbConnect)
                                callback.second(client);
                            for (auto& callback : _cbReceive)
                                callback.second(client, buffer);
                        }
                    }
                    else
                    {
                        for (auto& callback : _cbReceive)
                            callback.second(client, buffer);
                    }
                }
                else if (recv)
                {
                }
                else
                {
                }
            }
            else {
                std::cerr << "Failed to receive data: " << WSAGetLastError() << std::endl;
            }
        }
        else if (activity == 0) {
        }
        else {
            std::cerr << "Select error: " << WSAGetLastError() << std::endl;
        }
    }
    threadsCompleted.fetch_sub(1, std::memory_order_release);
}
