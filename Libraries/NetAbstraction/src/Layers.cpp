#include "netAbstraction/Layers.h"

#include <iostream>
#include <map>
#include <unordered_map>
#include <thread>
#include <chrono>

#define NOT_USE_NUMBERED_PACKET 0

// Define a custom hash function for SOCKET
namespace std {
    template<>
    struct hash<SOCKET> {
        size_t operator()(const SOCKET& sock) const {
            return static_cast<size_t>(sock); // Ensure it's hashable
        }
    };
}

// Ensure Address is hashable
namespace std {
    template<>
    struct hash<Layer::Address> {
        size_t operator()(const Layer::Address& addr) const {
            return std::hash<std::string>()(addr.ip) ^ std::hash<int>()(addr.port);
        }
    };
}

/**********************
 *    Common
 **********************/

bool Layer::initialize()
{
    return net_initialize();
}

void Layer::release()
{
    net_release();
}

void Layer::closeSocket(Layer::Socket& sock)
{
    net_close(sock);
}

/**********************
 *    TCP
 **********************/

bool LayerTCP::openSocket(int port, Layer::Socket& sock, const std::string& intf)
{
    if (!net_open(sock, NetTCP)) return false;
    if (!net_bindintf(sock, intf, port)) { net_close(sock); return false; }
    if (!net_listen(sock)) { net_close(sock); return false; }
    return true;
}

bool LayerTCP::acceptClient(Layer::Socket srv, Layer::Socket& cli, Layer::Address& addr)
{
    if (srv == InvalidSocket) return false;
    if (!net_accept(srv, cli, addr.ip, addr.port)) return false;
    return true;

}

bool LayerTCP::connectTo(Layer::Socket& sock, const Layer::Address& addr)
{
    if (!net_open(sock, NetTCP)) return false;
    if (!net_connect(sock,addr.ip, addr.port)) { net_close(sock); return false; }
    return true;
}

bool LayerTCP::send(const Layer::Socket& socket, const std::vector<char>& data, const Layer::Address& client_addr)
{
    unsigned int dataLength = static_cast<unsigned int>(data.size());

    unsigned int remainingBytes = dataLength;
    unsigned int currentIndex = 0;

    int bytesSent = net_send(socket, (char*)&dataLength, sizeof(dataLength), 0);
    while (remainingBytes > 0)
    {
        int toSend = remainingBytes > MaximumPacketSize(socket) ? MaximumPacketSize(socket) : remainingBytes;
        int bytesSent = net_send(socket, (char*)(data.data()) + currentIndex, toSend, 0);
        if (bytesSent <= 0) {
            return false;
        }
        if (bytesSent != toSend) {
            return false;
        }
        std::this_thread::sleep_for(std::chrono::microseconds(10));
        currentIndex += toSend;
        remainingBytes -= toSend;
    }
    return true;
}

bool LayerTCP::receive(const Layer::Socket& socket, std::vector<char>& data, Layer::Address& client_addr)
{
    unsigned int receivedBytes = 0;
    unsigned int transmittedLength = 0;
    unsigned int remainingBytes = 0;

    int bytesReceived = net_recv(socket, (char*)&transmittedLength, sizeof(transmittedLength), 0);
    data.resize(transmittedLength);
    receivedBytes = 0;
    remainingBytes = transmittedLength;
    while (receivedBytes < transmittedLength)
    {
        unsigned int toReceive = remainingBytes > MaximumPacketSize(socket) ? MaximumPacketSize(socket) : remainingBytes;
        int bytesReceived = net_recv(socket, data.data() + receivedBytes, toReceive, 0);
        if (bytesReceived == 0) {
            std::this_thread::sleep_for(std::chrono::microseconds(10));  // Small delay before retrying
            continue;
        }
        else if (bytesReceived < 0) {
            return false;
        }
        if (bytesReceived != toReceive) {
            std::cerr << "Too many byte received : " << bytesReceived << " : " << toReceive << std::endl;
            return false;
        }
        receivedBytes += bytesReceived;
        remainingBytes -= bytesReceived;
    }
    return true;
}

bool LayerTCP::partial_receive(const Layer::Socket& socket, std::vector<char>& data, Layer::Address& client_addr)
{
    static std::unordered_map<Layer::Socket, std::vector<char>> packetsPerSender;
    static std::unordered_map<Layer::Socket, unsigned int> receivedBytesPerSender;
    static std::unordered_map < Layer::Socket, unsigned int > transmittedBytesPerSender;
    data.clear();

    if (packetsPerSender.find(socket) == packetsPerSender.end())
    {
        unsigned int length;
        int bytes = net_recv(socket, (char*)&length, sizeof(unsigned int), 0);
        if (bytes == 0)
            return true;
        else if (bytes < 0)
            return false;
        if (bytes != 4)
            return false;
        std::vector<char> buffer(length);
        receivedBytesPerSender[socket] = 0;
        transmittedBytesPerSender[socket] = length;
        packetsPerSender[socket] = buffer;
    }
    else
    {
        unsigned int totalLength = transmittedBytesPerSender[socket];
        unsigned int currentIndex = receivedBytesPerSender[socket];
        unsigned int remainingBytes = totalLength - currentIndex;
        if (remainingBytes > MaximumPacketSize(socket)) remainingBytes = MaximumPacketSize(socket);
        int bytesReceived = net_recv(socket, (char*)(packetsPerSender[socket].data()) + currentIndex, remainingBytes, 0);
        currentIndex += bytesReceived;
        receivedBytesPerSender[socket] = currentIndex;
        if (currentIndex < totalLength)
            return true;
        if (currentIndex > totalLength)
            return false;
        data.resize(totalLength);
        std::memcpy(data.data(), packetsPerSender[socket].data(), totalLength);
        packetsPerSender.erase(socket);
        receivedBytesPerSender.erase(socket);
        transmittedBytesPerSender.erase(socket);
    }
    return true;
}

/**********************
 *    UDP
 **********************/

bool LayerUDP::openUnicastSocket(int port, Layer::Socket& sock, const std::string& intf)
{
    if (!net_open(sock, NetUDP)) return false;
    if (!net_reuse(sock)) { net_close(sock); return false; }
    if (!net_nonblocking(sock)) { net_close(sock); return false; }
    if (intf == "")
    {
        if (!net_bindany(sock, port)) { net_close(sock); return false; }
    }
    else
    {
        if (!net_bindintf(sock, intf, port)) { net_close(sock); return false; }
    }
    return true;
}

bool LayerUDP::openBroadcastSocket(int port, Layer::Socket& sock, const std::string& intf)
{
    if (!net_open(sock, NetUDP)) return false;
    if (!net_reuse(sock)) { net_close(sock); return false; }
    if (!net_nonblocking(sock)) { net_close(sock); return false; }
    if (!net_broadcast(sock)) { net_close(sock); return false; }
    if (intf=="")
    {
        if (!net_bindany(sock, port)) { net_close(sock); return false; }
    }
    else
    {
        if (!net_bindintf(sock, intf, port)) { net_close(sock); return false; }
    }
    return true;
}

bool LayerUDP::connectTo(Layer::Socket& sock, const Layer::Address& addr)
{
    if (!net_open(sock, NetUDP)) return false;
    if (!net_bindip(sock, addr.ip, 0)) { net_close(sock); return false; }
    return true;
}

bool LayerUDP::send(const Layer::Socket& socket, const std::vector<char>& data, const Layer::Address& client_addr, bool fake)
{
    unsigned int dataLength = static_cast<unsigned int>(data.size());

    unsigned int remainingBytes = dataLength;
    unsigned int currentIndex = 0;

    int bytesSent = net_sendto(socket, (char*) & dataLength, sizeof(dataLength), 0, client_addr.ip, client_addr.port);
    while (remainingBytes > 0)
    {
        int toSend = remainingBytes > MaximumPacketSize(socket) ? MaximumPacketSize(socket) : remainingBytes;
        int bytesSent = net_sendto(socket, (char*)(data.data())+currentIndex, toSend, 0, client_addr.ip, client_addr.port);
        if (bytesSent <= 0) {
            return false;
        }
        if (bytesSent != toSend) {
            return false;
        }
        std::this_thread::sleep_for(std::chrono::microseconds(10));
        currentIndex += toSend;
        remainingBytes -= toSend;
    }
    return true;
}

bool LayerUDP::receive(const Layer::Socket& socket, std::vector<char>& data, Layer::Address& client_addr)
{
    unsigned int receivedBytes = 0;
    unsigned int transmittedLength = 0;
    unsigned int remainingBytes = 0;

    int bytesReceived = net_recvfrom(socket, (char*)&transmittedLength, sizeof(transmittedLength), 0, client_addr.ip, client_addr.port);

    data.resize(transmittedLength);
    receivedBytes  = 0;
    remainingBytes = transmittedLength;
    while (receivedBytes < transmittedLength)
    {
        unsigned int toReceive = remainingBytes > MaximumPacketSize(socket) ? MaximumPacketSize(socket) : remainingBytes;
        int bytesReceived = net_recvfrom(socket, data.data()+ receivedBytes, toReceive, 0, client_addr.ip, client_addr.port);
        if (bytesReceived == 0) {
            std::this_thread::sleep_for(std::chrono::microseconds(10));  // Small delay before retrying
            continue;
        }
        else if (bytesReceived < 0) {
            return false;
        }
        if (bytesReceived != toReceive) {
            std::cerr << "Too many byte received : " << bytesReceived << " : " << toReceive << std::endl;
            return false;
        }
        receivedBytes += bytesReceived;
        remainingBytes -= bytesReceived;
    }
    return true;
}

bool LayerUDP::partial_receive(const Layer::Socket& socket, std::vector<char>& data, Layer::Address& client_addr)
{
    static std::unordered_map < Layer::Socket, std::unordered_map<Layer::Address, std::vector<char> > > packetsPerSender;
    static std::unordered_map < Layer::Socket, std::unordered_map<Layer::Address, unsigned int > > receivedBytesPerSender;
    static std::unordered_map < Layer::Socket, std::unordered_map<Layer::Address, unsigned int > > transmittedBytesPerSender;
    Layer::Address sender;
    data.clear();
    std::vector<char> buffer(MaximumPacketSize(socket));
    int bytes = net_recvfrom(socket, buffer.data(), MaximumPacketSize(socket), MSG_PEEK, sender.ip, sender.port);
    if (bytes == 0) return true;
    else if (bytes < 0) return false;

    if (packetsPerSender[socket].find(sender) == packetsPerSender[socket].end())
    {
        if (bytes != 4)
        {
            return false;
        }
        unsigned int length;
        int bytesReceived = net_recvfrom(socket, (char*)&length, bytes, 0, client_addr.ip, client_addr.port);
        buffer.resize(length);
        receivedBytesPerSender[socket][sender] = 0;
        transmittedBytesPerSender[socket][sender] = length;
        packetsPerSender[socket][sender] = buffer;
    }
    else
    {
        unsigned int totalLength = transmittedBytesPerSender[socket][sender];
        unsigned int currentIndex = receivedBytesPerSender[socket][sender];
        int bytesReceived = net_recvfrom(socket, (char*)(packetsPerSender[socket][sender].data()) + currentIndex, bytes, 0, client_addr.ip, client_addr.port);
        currentIndex += bytesReceived;
        receivedBytesPerSender[socket][sender] = currentIndex;
        if (currentIndex < totalLength)
        {
            return true;
        }
        if (currentIndex > totalLength)
        {
            return false;
        }
        data.resize(totalLength);
        std::memcpy(data.data(), packetsPerSender[socket][sender].data(), totalLength);
        packetsPerSender[socket].erase(sender);
        receivedBytesPerSender[socket].erase(sender);
        transmittedBytesPerSender[socket].erase(sender);
    }
    return true;
}
