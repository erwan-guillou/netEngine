#pragma once

#include <vector>
#include <string>

#include "netAbstraction/System.h"

class Layer
{
public:
    using Socket = NetSocket;
    struct Address {
        std::string ip;
        int port;

        Address() : ip(""), port(-1) {}
        bool operator==(const Address& other) const {
            return ip == other.ip && port == other.port;
        }
        bool operator<(const Address& other) const {
            return (ip < other.ip) || (ip == other.ip && port < other.port);
        }
        std::string toString() const { return ip + ":" + std::to_string(port); }
    };
    static bool initialize();
    static void release();
    static void closeSocket(Layer::Socket& sock);
};

class LayerTCP : public Layer
{
public:
    static bool openSocket(int port, Layer::Socket& sock, const std::string& intf = "");
    static bool acceptClient(Layer::Socket srv, Layer::Socket& cli, Layer::Address& addr);
    static bool connectTo(Layer::Socket& sock, const Layer::Address& addr);
    static bool send(const Layer::Socket& socket, const std::vector<char>& data, const Layer::Address& client_addr);
    static bool receive(const Layer::Socket& socket, std::vector<char>& data, Layer::Address& client_addr);
    static bool partial_receive(const Layer::Socket& socket, std::vector<char>& data, Layer::Address& client_addr);
};

class LayerUDP : public Layer
{
public:
    static bool openUnicastSocket(int port, Layer::Socket& sock, const std::string& intf = "");
    static bool openBroadcastSocket(int port, Layer::Socket& sock, const std::string& intf = "");
    static bool connectTo(Layer::Socket& sock, const Layer::Address& addr);
    static bool send(const Layer::Socket& socket, const std::vector<char>& data, const Layer::Address& client_addr, bool fake = false);
    static bool receive(const Layer::Socket& socket, std::vector<char>& data, Layer::Address& client_addr);
    static bool partial_receive(const Layer::Socket& socket, std::vector<char>& data, Layer::Address& client_addr);
};
