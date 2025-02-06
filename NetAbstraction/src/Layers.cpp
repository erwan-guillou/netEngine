#include "netAbstraction/Layers.h"

#include <iostream>
#include <map>
#include <unordered_map>

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
 *    UTILITY
 **********************/

std::string WStringToString(const std::wstring& wstr) {
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, NULL, 0, NULL, NULL);
    std::string str(size_needed-1, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &str[0], size_needed-1, NULL, NULL);
    return str;
}

Layer::Address convert(struct sockaddr_in clientAddr)
{
    char saddr[20];
    Layer::Address addr;
    inet_ntop(AF_INET, &(clientAddr.sin_addr), saddr, 20);
    addr.ip = std::string(saddr);
    addr.port = ntohs(clientAddr.sin_port);
    return addr;
}

void convert(const Layer::Address& addr, struct sockaddr_in& clientAddr)
{
    clientAddr.sin_family = AF_INET;
    clientAddr.sin_port = htons(addr.port);
    inet_pton(AF_INET, addr.ip.c_str(), &(clientAddr.sin_addr));
}

/**********************
 *    Structures
 **********************/

#pragma pack(push, 1)
union NetPacket {
    struct {
        unsigned short packetId;
        unsigned short payloadLength;
        char  payload[MaximumPayload];
    } data;
    char buffer[MaximumPacketSize];
};
#pragma pack(pop)

std::vector<NetPacket> split(const std::vector<char> message)
{
    std::vector<NetPacket> packets;

    unsigned int dataLength = static_cast<unsigned int>(message.size());
    std::vector<char> buffer(sizeof(dataLength) + message.size());
    std::memcpy(buffer.data(), &dataLength, sizeof(dataLength));
    std::memcpy(buffer.data() + sizeof(dataLength), message.data(), message.size());

    unsigned int remainingBytes = buffer.size();
    unsigned int currentIndex = 0;
    unsigned short packetId = 0;
    while (remainingBytes > 0)
    {
        NetPacket packet;
        packet.data.packetId = packetId;
        packet.data.payloadLength = remainingBytes > MaximumPayload ? MaximumPayload : remainingBytes;
        std::memcpy(packet.data.payload, buffer.data() + currentIndex, packet.data.payloadLength);
        packets.push_back(packet);
        packetId++;
        currentIndex += packet.data.payloadLength;
        remainingBytes -= packet.data.payloadLength;
    }

    return packets;
}

unsigned int dataLength(std::unordered_map<int, NetPacket> packets)
{
    if (packets.find(0) == packets.end()) return 0;
    const NetPacket& p0 = packets[0];
    unsigned int dataLength = *((unsigned int*)(p0.data.payload));
    return dataLength + sizeof(unsigned int);
}

std::vector<char> merge(std::unordered_map<int, NetPacket> packets)
{
    if (packets.empty()) return {};
    if (packets.find(0) == packets.end()) return {};

    unsigned int totalSize = dataLength(packets);

    // Allocate buffer for reconstructed message
    std::vector<char> buffer(totalSize);
    unsigned int currentIndex = 0;
    for (int packetId = 0; packets.find(packetId) != packets.end(); packetId++)
    {
        const NetPacket& packet = packets[packetId];
        // Calculate number of bytes to copy from this packet
        unsigned int bytesToCopy = packet.data.payloadLength;
        if (currentIndex + bytesToCopy > totalSize)
            bytesToCopy = totalSize - currentIndex; // Prevent overflow
        std::memcpy(buffer.data() + currentIndex, packet.data.payload, bytesToCopy);
        currentIndex += bytesToCopy;
    }

    // Remove the first 4 bytes (message length encoding)
    buffer.erase(buffer.begin(), buffer.begin() + sizeof(unsigned int));

    return buffer;
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

/*
int Layer::select(const Layer::Socket& socket, bool& readSet)
{
    const int timeoutSec = 0;
    const int timeoutMS = 10;

    fd_set rs;

    FD_ZERO(&rs);
    FD_SET(socket, &rs);

    // Set up the timeout structure
    timeval timeout;
    timeout.tv_sec = timeoutSec;
    timeout.tv_usec = timeoutMS * 1000;  // Convert milliseconds to microseconds

    // Use select to block for the specified timeout
    int result = ::select(0, &rs, nullptr, nullptr, &timeout);
    readSet = FD_ISSET(socket, &rs);
    return result;
}

int Layer::select(const std::vector<Layer::Socket>& sockets, std::vector<bool>& readSet)
{
    const int timeoutSec = 0;
    const int timeoutMS = 10;
    fd_set rs;
    FD_ZERO(&rs);

    int maxFd = 0;
    {

        // Populate the fd_set
        for (auto& s : sockets)
        {
            FD_SET(s, &rs);
            if (s > maxFd) maxFd = (int)s; // Track max socket value
        }
    }
    if (maxFd == 0) return 0;
    // Use select() with 10ms timeout
    timeval timeout;
    timeout.tv_sec = timeoutSec;
    timeout.tv_usec = timeoutMS * 1000;  // Convert milliseconds to microseconds

    int selectResult = ::select(maxFd + 1, &rs, nullptr, nullptr, &timeout);
    readSet = fdSetToVector(rs, maxFd);
    return selectResult;
}
*/

/*
std::vector<std::string> Layer::GetAdapterList()
{
    std::vector<std::string> result;

    ULONG outBufLen = 15000;
    PIP_ADAPTER_ADDRESSES pAdapterAddresses = (IP_ADAPTER_ADDRESSES*)malloc(outBufLen);

    if (GetAdaptersAddresses(AF_INET, GAA_FLAG_INCLUDE_PREFIX, NULL, pAdapterAddresses, &outBufLen) != ERROR_SUCCESS) {
        std::cerr << "Failed to get adapter addresses." << std::endl;
        free(pAdapterAddresses);
        return result;
    }
    for (PIP_ADAPTER_ADDRESSES adapter = pAdapterAddresses; adapter != NULL; adapter = adapter->Next) {
        std::string adapterName = WStringToString(adapter->FriendlyName);
        result.push_back(adapterName);
        //std::wcout << L"Interface: " << adapter->FriendlyName << L"\n";
        //std::cout << "MTU: " << adapter->Mtu << " bytes\n\n";
    }
    free(pAdapterAddresses);
    return result;
}

std::string Layer::getAdapterAddress(const std::string& adapterName)
{
    ULONG outBufLen = 15000;
    PIP_ADAPTER_ADDRESSES pAdapterAddresses = (IP_ADAPTER_ADDRESSES*)malloc(outBufLen);
    if (GetAdaptersAddresses(AF_INET, GAA_FLAG_INCLUDE_PREFIX, NULL, pAdapterAddresses, &outBufLen) != ERROR_SUCCESS) {
        std::cerr << "Failed to get adapter addresses." << std::endl;
        free(pAdapterAddresses);
        return "";
    }

    bool adapterFound = false;
    for (PIP_ADAPTER_ADDRESSES adapter = pAdapterAddresses; adapter != NULL; adapter = adapter->Next) {
        std::string currentAdapterName = WStringToString(adapter->FriendlyName);
        if (currentAdapterName == adapterName) {
            if (adapter->FirstUnicastAddress && adapter->FirstUnicastAddress->Address.lpSockaddr->sa_family == AF_INET) {
                sockaddr_in* sa = (sockaddr_in*)adapter->FirstUnicastAddress->Address.lpSockaddr;
                DWORD ip = sa->sin_addr.S_un.S_addr;  // IP address

                in_addr broadcastAddr;
                broadcastAddr.S_un.S_addr = ip;

                free(pAdapterAddresses);
                char saddr[20];
                inet_ntop(AF_INET, &(broadcastAddr), saddr, 20);
                return saddr;
            }
        }
    }

    free(pAdapterAddresses);
    std::cerr << "Specified adapter not found or has no valid IPv4 address." << std::endl;
    return "";
}

std::string Layer::ComputeBroadcastAddress(const std::string& ipStr)
{
    DWORD dwSize = 0;
    GetAdaptersAddresses(AF_INET, 0, NULL, NULL, &dwSize);
    IP_ADAPTER_ADDRESSES* pAdapterAddresses = (IP_ADAPTER_ADDRESSES*)malloc(dwSize);

    if (GetAdaptersAddresses(AF_INET, 0, NULL, pAdapterAddresses, &dwSize) == NO_ERROR) {
        for (IP_ADAPTER_ADDRESSES* pAdapter = pAdapterAddresses; pAdapter; pAdapter = pAdapter->Next) {
            for (IP_ADAPTER_UNICAST_ADDRESS* pUnicast = pAdapter->FirstUnicastAddress; pUnicast; pUnicast = pUnicast->Next) {
                SOCKADDR_IN* pSockAddr = (SOCKADDR_IN*)pUnicast->Address.lpSockaddr;
                char strAddr[INET_ADDRSTRLEN] = { 0 };
                inet_ntop(AF_INET, &pSockAddr->sin_addr, strAddr, INET_ADDRSTRLEN);

                if (ipStr == strAddr) { // Match found
                    in_addr ipAddr, subnetMask;
                    ipAddr.s_addr = pSockAddr->sin_addr.s_addr;
                    subnetMask.s_addr = pUnicast->OnLinkPrefixLength ? htonl(~((1 << (32 - pUnicast->OnLinkPrefixLength)) - 1)) : 0;

                    in_addr broadcastAddr;
                    broadcastAddr.s_addr = ipAddr.s_addr | ~subnetMask.s_addr;

                    char broadcastStr[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, &broadcastAddr, broadcastStr, INET_ADDRSTRLEN);

                    free(pAdapterAddresses);
                    return std::string(broadcastStr);
                }
            }
        }
    }

    std::cerr << "Interface not found or no IPv4 address assigned." << std::endl;
    free(pAdapterAddresses);
    return "";
}

std::string Layer::GetBroadcastAddress(const std::string& interfaceName) {
    ULONG outBufLen = 15000;
    PIP_ADAPTER_ADDRESSES pAdapterAddresses = (IP_ADAPTER_ADDRESSES*)malloc(outBufLen);

    if (GetAdaptersAddresses(AF_INET, GAA_FLAG_INCLUDE_PREFIX, NULL, pAdapterAddresses, &outBufLen) != ERROR_SUCCESS) {
        std::cerr << "Failed to get adapter addresses." << std::endl;
        free(pAdapterAddresses);
        return "";
    }

    for (PIP_ADAPTER_ADDRESSES adapter = pAdapterAddresses; adapter != NULL; adapter = adapter->Next) {
        std::string adapterName = WStringToString(adapter->FriendlyName);
        if (adapter->FriendlyName && interfaceName == adapterName) {
            for (PIP_ADAPTER_UNICAST_ADDRESS unicast = adapter->FirstUnicastAddress; unicast != NULL; unicast = unicast->Next) {
                sockaddr_in* sa = (sockaddr_in*)unicast->Address.lpSockaddr;
                DWORD ip = sa->sin_addr.S_un.S_addr;  // IP address

                // Get subnet mask from adapter
                ULONG mask;
                ConvertLengthToIpv4Mask(unicast->OnLinkPrefixLength, &mask);

                // Compute broadcast address
                DWORD broadcast = ip | ~mask;

                in_addr broadcastAddr;
                broadcastAddr.S_un.S_addr = broadcast;

                free(pAdapterAddresses);
                char saddr[20];
                inet_ntop(AF_INET, &(broadcastAddr), saddr, 20);
                return saddr;
            }
        }
    }
    std::cerr << "Interface not found or no IPv4 address assigned." << std::endl;
    free(pAdapterAddresses);
    return "";
}
*/
/**********************
 *    TCP
 **********************/

bool LayerTCP::openSocket(int port, Layer::Socket& sock, const std::string& intf)
{
    if (!net_open(sock, NetTCP)) return false;
    if (!net_bindintf(sock, intf, port)) { net_close(sock); return false; }
    if (!net_listen(sock)) { net_close(sock); return false; }
    /*
    sockaddr_in serverAddr = {};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (intf != "")
    {
        Layer::Address addr;
        addr.ip = Layer::getAdapterAddress(intf);
        addr.port = port;
        if (addr.ip == "")
        {
            std::cerr << "local ip address of interface not found" << std::endl;
            return false;
        }
        convert(addr, serverAddr);
    }

    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl;
        return false;
    }


    if (bind(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed: " << WSAGetLastError() << std::endl;
        return false;
    }

    if (listen(sock, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed: " << WSAGetLastError() << std::endl;
        return false;
    }
    */
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
    /*
    sockaddr_in clientAddr;
    int clientAddrSize = sizeof(clientAddr);
    convert(addr, clientAddr);
    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        std::cerr << "Error creating socket: " << WSAGetLastError() << std::endl;
        return false;
    }

    // Connect to the server
    if (::connect(sock, (sockaddr*)&clientAddr, sizeof(clientAddr)) == SOCKET_ERROR) {
        std::cerr << "Error connecting to server: " << WSAGetLastError() << std::endl;
        closeSocket(sock);
        return false;
    }
    */
    return true;
}

bool LayerTCP::send(const Layer::Socket& socket, const std::vector<char>& data, const Layer::Address& client_addr)
{
    std::vector<NetPacket> packets;
    NetPacket packet;

    unsigned int dataLength = static_cast<unsigned int>(data.size());
    std::vector<char> buffer(sizeof(dataLength) + data.size());
    std::memcpy(buffer.data(), &dataLength, sizeof(dataLength));
    std::memcpy(buffer.data() + sizeof(dataLength), data.data(), data.size());

    unsigned int remainingBytes = buffer.size();
    unsigned int currentIndex = 0;
    unsigned short packetId = 0;
    while (remainingBytes > 0)
    {
        packet.data.packetId = packetId;
        packet.data.payloadLength = remainingBytes > MaximumPayload ? MaximumPayload : remainingBytes;
        std::memcpy(packet.data.payload, buffer.data() + currentIndex, packet.data.payloadLength);

        int toSend = packet.data.payloadLength + 2 * sizeof(unsigned short);
        int bytesSent = net_send(socket, packet.buffer, toSend, 0);
        if (bytesSent <= 0) return false;
        if (bytesSent != toSend) return false;

        packetId++;
        currentIndex += packet.data.payloadLength;
        remainingBytes -= packet.data.payloadLength;
    }
    return true;
}

bool LayerTCP::receive(const Layer::Socket& socket, std::vector<char>& data, Layer::Address& client_addr)
{
    while (true) {
        if (!LayerTCP::partial_receive(socket, data, client_addr))
        {
            return false;
        }
        if (data.size() > 0) break;
    }
    return true;
}

bool LayerTCP::partial_receive(const Layer::Socket& socket, std::vector<char>& data, Layer::Address& client_addr)
{
    static std::unordered_map<Layer::Socket, std::unordered_map<int, NetPacket>> packetsPerSender;
    static std::unordered_map<Layer::Socket, unsigned int> receivedBytesPerSender;
    data.clear();
    NetPacket packet;
    int bytesReceivedId = net_recv(socket, packet.buffer, sizeof(unsigned short), 0);
    if (bytesReceivedId != sizeof(unsigned short)) return false;
    int bytesReceivedSz = net_recv(socket, packet.buffer + sizeof(unsigned short), sizeof(unsigned short), 0);
    if (bytesReceivedSz != sizeof(unsigned short)) return false;
    int bytesReceived = net_recv(socket, packet.buffer + 2 * sizeof(unsigned short), packet.data.payloadLength, 0);
    int toReceive = packet.data.payloadLength;
    if (bytesReceived <= 0) return false;
    if (bytesReceived != toReceive) return false;

    if (packetsPerSender.find(socket) == packetsPerSender.end())
    {
        packetsPerSender[socket] = std::unordered_map<int, NetPacket>();
        receivedBytesPerSender[socket] = 0;
    }
    packetsPerSender[socket][packet.data.packetId] = packet;
    receivedBytesPerSender[socket] += packet.data.payloadLength;
    unsigned int transmittedLength = dataLength(packetsPerSender[socket]);
    if (transmittedLength <= 0) return true;
    if (receivedBytesPerSender[socket] < transmittedLength) return true;
    if (receivedBytesPerSender[socket] > transmittedLength) return false;
    data = merge(packetsPerSender[socket]);
    packetsPerSender.erase(socket);
    receivedBytesPerSender.erase(socket);
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
    std::vector<NetPacket> packets;
    NetPacket packet;

    unsigned int dataLength = static_cast<unsigned int>(data.size());
    std::vector<char> buffer(sizeof(dataLength) + data.size());
    std::memcpy(buffer.data(), &dataLength, sizeof(dataLength));
    std::memcpy(buffer.data() + sizeof(dataLength), data.data(), data.size());

    unsigned int remainingBytes = buffer.size();
    unsigned int currentIndex = 0;
    unsigned short packetId = 0;
    while (remainingBytes > 0)
    {
        packet.data.packetId = packetId;
        packet.data.payloadLength = remainingBytes > MaximumPayload ? MaximumPayload : remainingBytes;
        std::memcpy(packet.data.payload, buffer.data() + currentIndex, packet.data.payloadLength);

        if (!fake)
        {
            int toSend = packet.data.payloadLength + 2 * sizeof(unsigned short);
            int bytesSent = net_sendto(socket, packet.buffer, toSend, 0, client_addr.ip, client_addr.port);
            if (bytesSent <= 0) return false;
            if (bytesSent != toSend) return false;
        }

        packetId++;
        currentIndex += packet.data.payloadLength;
        remainingBytes -= packet.data.payloadLength;
    }
    return true;
}

bool LayerUDP::receive(const Layer::Socket& socket, std::vector<char>& data, Layer::Address& client_addr)
{
    std::unordered_map<int, NetPacket> packets;
    unsigned int receivedBytes = 0;
    while (true)
    {
        NetPacket packet;
        int bytesReceived = net_recvfrom(socket, packet.buffer, MaximumPacketSize, 0, client_addr.ip, client_addr.port);
        int toReceive = packet.data.payloadLength + 2 * sizeof(unsigned short);
        if (bytesReceived <= 0) return false;
        if (bytesReceived != toReceive) return false;
        packets[packet.data.packetId] = packet;
        receivedBytes += packet.data.payloadLength;
        unsigned int transmittedLength = dataLength(packets);
        if (receivedBytes == transmittedLength) break;
    }
    data = merge(packets);
    return true;
}

bool LayerUDP::partial_receive(const Layer::Socket& socket, std::vector<char>& data, Layer::Address& client_addr)
{
    static std::unordered_map < Layer::Socket, std::unordered_map<Layer::Address, std::unordered_map<int, NetPacket> > > packetsPerSender;
    static std::unordered_map < Layer::Socket, std::unordered_map<Layer::Address, unsigned int > > receivedBytesPerSender;
    data.clear();
    NetPacket packet;
    int bytesReceived = net_recvfrom(socket, packet.buffer, MaximumPacketSize, 0, client_addr.ip, client_addr.port);
    int toReceive = packet.data.payloadLength + 2 * sizeof(unsigned short);
    if (bytesReceived <= 0) return false;
    if (bytesReceived != toReceive) return false;

    if (packetsPerSender[socket].find(client_addr) == packetsPerSender[socket].end())
    {
        packetsPerSender[socket][client_addr] = std::unordered_map<int, NetPacket>();
        receivedBytesPerSender[socket][client_addr] = 0;
    }
    packetsPerSender[socket][client_addr][packet.data.packetId] = packet;
    receivedBytesPerSender[socket][client_addr] += packet.data.payloadLength;
    unsigned int transmittedLength = dataLength(packetsPerSender[socket][client_addr]);
    if (transmittedLength <= 0) return true;
    if (receivedBytesPerSender[socket][client_addr] < transmittedLength) return true;
    if (receivedBytesPerSender[socket][client_addr] > transmittedLength) return false;

    data = merge(packetsPerSender[socket][client_addr]);
    packetsPerSender[socket].erase(client_addr);
    receivedBytesPerSender[socket].erase(client_addr);
    return true;
}
