#include "netAbstraction/ServerTCP.h"

#include <thread>
#include <iostream>

template <typename K, typename V>
std::vector<K> getKeys(const std::map<K, V>& m) {
    std::vector<K> keys;
    keys.reserve(m.size()); // Optimize memory allocation
    for (const auto& pair : m) {
        keys.push_back(pair.first);
    }
    return keys;
}

ServerTCP::ServerTCP(const int basePort, const std::string& intf)
    : Server(basePort, intf)
{
}

ServerTCP::~ServerTCP()
{
}

bool ServerTCP::start() {
    if (startedFlag.load()) return false;

    stopFlag.store(false);
    std::thread(&ServerTCP::_acceptClients, this).detach();
    std::thread(&ServerTCP::_listenForMessages, this).detach();
    startedFlag.store(true);
    return true;
}

void ServerTCP::stop()
{
    if (!startedFlag.load()) return;
    stopFlag.store(true);
    while (threadsCompleted.load() > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    startedFlag.store(false);
}

bool ServerTCP::connect()
{
    if (isConnected.load()) return false;
    if (!LayerTCP::openSocket(_port, sock, intfToUse)) return false;
    isConnected.store(true);
    return true;
}

void ServerTCP::disconnect()
{
    if (!isConnected.load()) return;
    {
        std::lock_guard<std::mutex> lock(clientMutex);
        LayerTCP::closeSocket(sock);
    }
    isConnected.store(false);
}

void ServerTCP::send(const std::vector<char>& data) {
    std::vector<Layer::Address> _disconnected;
    std::map<Layer::Socket, Layer::Address> sockets;

    {
        std::lock_guard<std::mutex> lock(clientMutex);
        sockets = _fromSocket;
    }
    for (auto it = sockets.begin(); it != sockets.end(); ) {
        if (LayerTCP::send(_toSocket[it->second], data, it->second))
        {
            ++it;
        }
        else
        {
            _disconnected.push_back(it->second);
        }
    }
    // Process disconnected clients outside the lock
    for (size_t i = 0; i < _disconnected.size(); i++)
    {
        Layer::Address client = _disconnected[i];
        for (auto& callback : _cbDisconnect)
            callback.second(_disconnected[i]);
        {
            std::lock_guard<std::mutex> lock(clientMutex);
            Layer::Socket sock = _toSocket[client];
            _toSocket.erase(client);
            _fromSocket.erase(sock);
            auto it = std::find(_listenTo.begin(), _listenTo.end(), sock);
            if (it != _listenTo.end()) {
                _listenTo.erase(it);
            }
            LayerTCP::closeSocket(sock);
        }
    }
}

void ServerTCP::send(const Layer::Address& client, const std::vector<char>& data) {
    std::vector<Layer::Address> _disconnected;
    {
        std::lock_guard<std::mutex> lock(clientMutex);
        if (LayerTCP::send(_toSocket[client], data, client))
        {
        }
        else
        {
            _disconnected.push_back(client);
        }
    }
    // Process disconnected clients outside the lock
    for (size_t i = 0; i < _disconnected.size(); i++)
    {
        Layer::Address client = _disconnected[i];
        for (auto& callback : _cbDisconnect)
            callback.second(_disconnected[i]);
        {
            std::lock_guard<std::mutex> lock(clientMutex);
            Layer::Socket sock = _toSocket[client];
            _toSocket.erase(client);
            _fromSocket.erase(sock);
            auto it = std::find(_listenTo.begin(), _listenTo.end(), sock);
            if (it != _listenTo.end()) {
                _listenTo.erase(it);
            }
            LayerTCP::closeSocket(sock);
        }
    }
}

void ServerTCP::receive(const Layer::Address& client, std::vector<char>& buffer)
{
    std::vector<Layer::Address> _disconnected;
    {
        Layer::Address cli;
        Layer::Socket sock;
        {
            std::lock_guard<std::mutex> lock(clientMutex);
            sock = _toSocket[client];
        }
        if (!LayerTCP::receive(sock, buffer, cli))
        {
            _disconnected.push_back(client);
        }
    }
    // Process disconnected clients outside the lock
    for (size_t i = 0; i < _disconnected.size(); i++)
    {
        Layer::Address client = _disconnected[i];
        for (auto& callback : _cbDisconnect)
            callback.second(_disconnected[i]);
        {
            std::lock_guard<std::mutex> lock(clientMutex);
            Layer::Socket sock = _toSocket[client];
            _toSocket.erase(client);
            _fromSocket.erase(sock);
            auto it = std::find(_listenTo.begin(), _listenTo.end(), sock);
            if (it != _listenTo.end()) {
                _listenTo.erase(it);
            }
            LayerTCP::closeSocket(sock);
        }
    }
}

void ServerTCP::_acceptClients()
{
    threadsCompleted.fetch_add(1, std::memory_order_release);
    while (!stopFlag.load())
    {
        if (!isConnected.load()) continue;
        Layer::Address clientAddress;
        Layer::Socket clientSocket;

        // Use select to implement a timeout on the accept call
        bool readSet;
        int result = net_select(sock, readSet);

        if (result > 0 && readSet) {
            if (LayerTCP::acceptClient(sock,clientSocket,clientAddress))
            {
                {
                    std::lock_guard<std::mutex> lock(clientMutex);
                    _toSocket[clientAddress] = clientSocket;
                    _fromSocket[clientSocket] = clientAddress;
                }
                bool isGood = true;
                if (_cbHandshake)
                    isGood = _cbHandshake(clientAddress, std::vector<char>());
                if (isGood)
                {
                    for (auto& callback : _cbConnect)
                        callback.second(clientAddress);
                    {
                        std::lock_guard<std::mutex> lock(clientMutex);
                        _listenTo.push_back(clientSocket);
                    }
                }
                else
                {
                    std::lock_guard<std::mutex> lock(clientMutex);
                    Layer::Socket sock = _toSocket[clientAddress];
                    _toSocket.erase(clientAddress);
                    _fromSocket.erase(sock);
                    LayerTCP::closeSocket(sock);
                }
            }
            else {
                std::cerr << "Failed to accept client: " << WSAGetLastError() << std::endl;
            }
        }
        else if (result == 0) {
            // Timeout occurred
        }
        else {
            // Select failed
            std::cerr << "Select failed: " << WSAGetLastError() << std::endl;
        }
    }
    threadsCompleted.fetch_sub(1, std::memory_order_release);
}

void ServerTCP::_listenForMessages()
{
    threadsCompleted.fetch_add(1, std::memory_order_release);
    while (!stopFlag.load())
    {
        if (!isConnected.load()) continue;
        std::vector <Layer::Address> _from;
        std::vector<std::vector<char>> _data;
        std::vector<Layer::Address> _disconnected;

        std::vector<bool> readSet;
        std::map<Layer::Socket,Layer::Address> sockets;

        {
            std::lock_guard<std::mutex> lock(clientMutex);
            for (auto& it: _listenTo)
                sockets[it] = _fromSocket[it];
        }

        int selectResult;
        selectResult = net_select(getKeys(sockets), readSet);
        if (selectResult == SOCKET_ERROR)
        {
            std::cerr << "select() failed with error: " << WSAGetLastError() << std::endl;
        }
        if (selectResult > 0) // If sockets are ready for reading
        {
            for (auto it = sockets.begin(); it != sockets.end();)
            {
                Layer::Socket s = it->first;
                if (readSet[s]) // Check if socket is ready to read
                {
                    Layer::Address client = it->second;
                    std::vector<char> buffer;
                    Layer::Address cli;
                    if (!LayerTCP::partial_receive(_toSocket[client], buffer, cli)) // If receiving failed, disconnect
                    {
                        _disconnected.push_back(client);
                    }
                    else
                    {
                        if (buffer.size() > 0)
                        {
                            _from.push_back(client);
                            _data.push_back(buffer);
                        }
                    }
                }
                ++it;
            }
        }
        // Process disconnected clients outside the lock
        for (size_t i = 0; i < _disconnected.size(); i++)
        {
            Layer::Address client = _disconnected[i];
            for (auto& callback : _cbDisconnect)
                callback.second(_disconnected[i]);
            {
                std::lock_guard<std::mutex> lock(clientMutex);
                Layer::Socket sock = _toSocket[client];
                _toSocket.erase(client);
                _fromSocket.erase(sock);
                auto it = std::find(_listenTo.begin(), _listenTo.end(), sock);
                if (it != _listenTo.end()) {
                    _listenTo.erase(it);
                }
                LayerTCP::closeSocket(sock);
            }
        }
        // Process received messages outside the lock
        for (size_t i = 0; i < _from.size(); i++)
        {
            for (auto& callback : _cbReceive)
                callback.second(_from[i], _data[i]);
        }
    }
    threadsCompleted.fetch_sub(1, std::memory_order_release);
}
