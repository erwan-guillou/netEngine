#include "netAbstraction/System.h"

#include <iostream>

std::vector<std::string> GetAdapterList() {
    std::vector<std::string> result;

#ifdef _WIN32
    ULONG outBufLen = 15000;
    PIP_ADAPTER_ADDRESSES pAdapterAddresses = (IP_ADAPTER_ADDRESSES*)malloc(outBufLen);
    if (GetAdaptersAddresses(AF_INET, GAA_FLAG_INCLUDE_PREFIX, NULL, pAdapterAddresses, &outBufLen) != ERROR_SUCCESS) {
        std::cerr << "Failed to get adapter addresses." << std::endl;
        free(pAdapterAddresses);
        return result;
    }
    for (PIP_ADAPTER_ADDRESSES adapter = pAdapterAddresses; adapter != NULL; adapter = adapter->Next) {
        std::wstring wstr = adapter->FriendlyName;
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, NULL, 0, NULL, NULL);
        std::string adapterName(size_needed - 1, 0);
        WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &adapterName[0], size_needed, NULL, NULL);
        result.push_back(adapterName);
    }
    free(pAdapterAddresses);
#else
    struct ifaddrs* ifaddr;
    if (getifaddrs(&ifaddr) == -1) {
        std::cerr << "Failed to get network interfaces." << std::endl;
        return result;
    }
    for (struct ifaddrs* ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_name != NULL) {
            result.push_back(ifa->ifa_name);
        }
    }
    freeifaddrs(ifaddr);
#endif
    return result;
}

std::string GetAdapterAddress(const std::string& adapterName) {
#ifdef _WIN32
    ULONG outBufLen = 15000;
    PIP_ADAPTER_ADDRESSES pAdapterAddresses = (IP_ADAPTER_ADDRESSES*)malloc(outBufLen);
    if (GetAdaptersAddresses(AF_INET, GAA_FLAG_INCLUDE_PREFIX, NULL, pAdapterAddresses, &outBufLen) != ERROR_SUCCESS) {
        std::cerr << "Failed to get adapter addresses." << std::endl;
        free(pAdapterAddresses);
        return "";
    }

    bool adapterFound = false;
    for (PIP_ADAPTER_ADDRESSES adapter = pAdapterAddresses; adapter != NULL; adapter = adapter->Next) {
        const std::wstring& wstr = adapter->FriendlyName;
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, NULL, 0, NULL, NULL);
        std::string currentAdapterName(size_needed - 1, 0);
        WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &currentAdapterName[0], size_needed - 1, NULL, NULL);
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
#else
    struct ifaddrs* ifaddr;
    if (getifaddrs(&ifaddr) == -1) {
        std::cerr << "Failed to get network interfaces." << std::endl;
        return "";
    }
    for (struct ifaddrs* ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET && adapterName == ifa->ifa_name) {
            char ipStr[INET_ADDRSTRLEN];
            struct sockaddr_in* sa_in = (struct sockaddr_in*)ifa->ifa_addr;
            inet_ntop(AF_INET, &(sa_in->sin_addr), ipStr, INET_ADDRSTRLEN);
            freeifaddrs(ifaddr);
            return std::string(ipStr);
        }
    }
    freeifaddrs(ifaddr);
    return "";
#endif
}

std::string ComputeBroadcastAddress(const std::string& ipStr) {
    struct sockaddr_in addr;
    if (inet_pton(AF_INET, ipStr.c_str(), &addr.sin_addr) != 1) {
        std::cerr << "Invalid IP address format." << std::endl;
        return "";
    }

#ifdef _WIN32
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
#else
    struct ifaddrs* ifaddr;
    if (getifaddrs(&ifaddr) == -1) {
        std::cerr << "Failed to get network interfaces." << std::endl;
        return "";
    }
    for (struct ifaddrs* ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET) {
            struct sockaddr_in* sa_in = (struct sockaddr_in*)ifa->ifa_addr;
            struct sockaddr_in* netmask = (struct sockaddr_in*)ifa->ifa_netmask;
            if (sa_in->sin_addr.s_addr == addr.sin_addr.s_addr) {
                uint32_t broadcast = sa_in->sin_addr.s_addr | ~netmask->sin_addr.s_addr;
                char broadcastStr[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &broadcast, broadcastStr, INET_ADDRSTRLEN);
                freeifaddrs(ifaddr);
                return std::string(broadcastStr);
            }
        }
    }
    freeifaddrs(ifaddr);
#endif
    return "";
}

std::string GetBroadcastAddress(const std::string& interfaceName) {
#ifdef _WIN32
    ULONG outBufLen = 15000;
    PIP_ADAPTER_ADDRESSES pAdapterAddresses = (IP_ADAPTER_ADDRESSES*)malloc(outBufLen);

    if (GetAdaptersAddresses(AF_INET, GAA_FLAG_INCLUDE_PREFIX, NULL, pAdapterAddresses, &outBufLen) != ERROR_SUCCESS) {
        std::cerr << "Failed to get adapter addresses." << std::endl;
        free(pAdapterAddresses);
        return "";
    }

    for (PIP_ADAPTER_ADDRESSES adapter = pAdapterAddresses; adapter != NULL; adapter = adapter->Next) {
        const std::wstring& wstr = adapter->FriendlyName;
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, NULL, 0, NULL, NULL);
        std::string adapterName(size_needed - 1, 0);
        WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &adapterName[0], size_needed - 1, NULL, NULL);
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
#else
    struct ifaddrs* ifaddr;
    if (getifaddrs(&ifaddr) == -1) {
        std::cerr << "Failed to get network interfaces." << std::endl;
        return "";
    }
    for (struct ifaddrs* ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET && interfaceName == ifa->ifa_name) {
            struct sockaddr_in* sa_in = (struct sockaddr_in*)ifa->ifa_addr;
            struct sockaddr_in* netmask = (struct sockaddr_in*)ifa->ifa_netmask;
            uint32_t broadcast = sa_in->sin_addr.s_addr | ~netmask->sin_addr.s_addr;
            char broadcastStr[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &broadcast, broadcastStr, INET_ADDRSTRLEN);
            freeifaddrs(ifaddr);
            return std::string(broadcastStr);
        }
    }
    freeifaddrs(ifaddr);
    return "";
#endif
}

unsigned int MaximumPacketSize(NetSocket sock)
{
    int sendBufSize;
    socklen_t optlen = sizeof(sendBufSize);
    getsockopt(sock, SOL_SOCKET, SO_SNDBUF, (char*)&sendBufSize, &optlen);
    return sendBufSize - 100;
}

void net_error(const std::string message)
{
#ifdef _WIN32
    int errorCode = WSAGetLastError();
#else
    int errorCode = errno;
#endif
    std::cerr << "Error: " << message << " (Code: " << errorCode << ")" << std::endl;
}

bool net_initialize()
{
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed: " << WSAGetLastError() << std::endl;
        return false;
    }
#endif
    return true;
}

void net_release()
{
#ifdef _WIN32
    WSACleanup();
#endif
}

int net_select(const NetSocket& socket, bool& readSet)
{
    if (socket == InvalidSocket) return -1;

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
    int result = ::select((int)socket + 1, &rs, nullptr, nullptr, &timeout);

#ifdef _WIN32
    if (result == SOCKET_ERROR) {
#else
    if (result == -1) {
#endif
        net_error("Select failed");
        return -1;
    }

    readSet = FD_ISSET(socket, &rs);
    return result;
}

int net_select(const std::vector<NetSocket>& sockets, std::vector<bool>& readSet)
{
    const int timeoutSec = 0;
    const int timeoutMS = 10;
    fd_set rs;
    FD_ZERO(&rs);

    int maxFd = 0;

    // Populate the fd_set
    for (auto& s : sockets) {
        if (s == InvalidSocket) continue; // Skip invalid sockets
        FD_SET(s, &rs);
        if (s > maxFd) maxFd = (int)s; // Track max socket value
    }

    if (maxFd == 0) return 0; // No valid sockets

    // Use select() with 10ms timeout
    timeval timeout;
    timeout.tv_sec = timeoutSec;
    timeout.tv_usec = timeoutMS * 1000;  // Convert milliseconds to microseconds

    int selectResult = ::select(maxFd + 1, &rs, nullptr, nullptr, &timeout);

#ifdef _WIN32
    if (selectResult == SOCKET_ERROR) {
#else
    if (selectResult == -1) {
#endif
        net_error("Select failed");
        return -1;
    }

    readSet = std::vector<bool>(maxFd + 1, false);  // Boolean array indexed by FD values
    for (auto& s : sockets) {
        if (FD_ISSET(s, &rs)) {
            readSet[s] = true;
        }
    }

    return selectResult;
}

bool net_open(NetSocket& sock, unsigned char proto)
{
    switch (proto) {
    case NetTCP:
        sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        break;
    case NetUDP:
        sock = socket(AF_INET, SOCK_DGRAM, 0);
        break;
    default:
        net_error("Unknown protocol");
        return false;
    }

#ifdef _WIN32
    if (sock == INVALID_SOCKET) {
#else
    if (sock == -1) {
#endif
        net_error("Socket creation failed");
        return false;
    }

    return true;
}

void net_close(NetSocket& socket)
{
    if (socket == InvalidSocket) return;
#ifdef _WIN32
    closesocket(socket);
#else
    close(socket);
#endif
    socket = InvalidSocket;
}

bool net_reuse(const NetSocket& socket, bool reuse)
{
    if (socket == InvalidSocket) return false;
    bool hasError = false;
    int _reuse = reuse ? 1 : 0;
#ifdef _WIN32
    hasError = (setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, (char*)&_reuse, sizeof(_reuse)) == SOCKET_ERROR);
#else
    hasError = (setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, &_reuse, sizeof(_reuse)) == -1);
#endif
    if (hasError) {
        net_error("setsockopt(SO_REUSEADDR) failed!");
        return false;
    }
    return true;
}

bool net_nonblocking(const NetSocket & socket, bool nonblock)
{
    if (socket == InvalidSocket) return false;
    bool hasError = false;
#ifdef _WIN32
    u_long mode = nonblock ? 1 : 0;  // 1 = Non-blocking
    hasError = (ioctlsocket(socket, FIONBIO, &mode) != 0);
#else
    int flags = fcntl(socket, F_GETFL, 0);
    if (flags == -1) {
        net_error("Failed to get socket flags");
        return false;
    }
    if (nonblock) {
        flags |= O_NONBLOCK;
    }
    else {
        flags &= ~O_NONBLOCK;
    }
    hasError = (fcntl(socket, F_SETFL, flags) == -1);
#endif
    if (hasError) {
        net_error("Failed to set non-blocking mode");
        return false;
    }
    return true;
}

bool net_broadcast(const NetSocket& socket, bool bcast)
{
    if (socket == InvalidSocket) return false;
    bool hasError = false;
    int broadcastEnable = bcast ? 1 : 0;
#ifdef _WIN32
    hasError = (setsockopt(socket, SOL_SOCKET, SO_BROADCAST, (char*)&broadcastEnable, sizeof(broadcastEnable)) == SOCKET_ERROR);
#else
    hasError = (setsockopt(socket, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable)) == -1);
#endif
    if (hasError) {
        net_error("setsockopt(SO_BROADCAST) failed!");
        return false;
    }
    return true;
}

bool net_bindany(const NetSocket& socket, int port)
{
    if (socket == InvalidSocket) return false;
    bool hasError = false;

    sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
#ifdef _WIN32
    hasError = (bind(socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR);
#else
    hasError = (bind(socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1);
#endif

    if (hasError) {
        net_error("Binding failed!");
        return false;
    }
    return true;
}

bool net_bindintf(const NetSocket& socket, const std::string& intf, int port)
{
    if (socket == InvalidSocket) return false;
    if (intf.empty()) {
        net_error("No interface provided");
        return false;
    }

    sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    std::string ip = GetAdapterAddress(intf);
    if (ip.empty()) {
        net_error("Local IP address of interface not found");
        return false;
    }

    if (inet_pton(AF_INET, ip.c_str(), &(server_addr.sin_addr)) != 1) {
        net_error("Invalid IP address format");
        return false;
    }

    bool hasError = false;

#ifdef _WIN32
    hasError = (bind(socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR);
#else
    hasError = (bind(socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1);
#endif

    if (hasError) {
        net_error("Binding failed!");
        return false;
    }
    return true;
}

bool net_bindip(const NetSocket& socket, const std::string& ip, int port)
{
    if (socket == InvalidSocket) return false;
    if (ip.empty()) {
        net_error("No IP address provided");
        return false;
    }

    sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip.c_str(), &(server_addr.sin_addr)) != 1) {
        net_error("Invalid IP address format");
        return false;
    }

    bool hasError = false;

#ifdef _WIN32
    hasError = (bind(socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR);
#else
    hasError = (bind(socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1);
#endif

    if (hasError) {
        net_error("Binding failed!");
        return false;
    }

    return true;
}

bool net_listen(const NetSocket& socket)
{
    if (socket == InvalidSocket) return false;

    bool hasError = false;

#ifdef _WIN32
    hasError = (listen(socket, SOMAXCONN) == SOCKET_ERROR);
#else
    hasError = (listen(socket, SOMAXCONN) == -1);
#endif

    if (hasError) {
        net_error("Listen failed!");
        return false;
    }

    return true;
}

bool net_connect(const NetSocket& socket, const std::string& ip, int port)
{
    if (socket == InvalidSocket) return false;

    sockaddr_in clientAddr;
    memset(&clientAddr, 0, sizeof(clientAddr));
    clientAddr.sin_family = AF_INET;
    clientAddr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip.c_str(), &(clientAddr.sin_addr)) != 1) {
        net_error("Invalid IP address format");
        return false;
    }

    bool hasError = false;

#ifdef _WIN32
    hasError = (::connect(socket, (sockaddr*)&clientAddr, sizeof(clientAddr)) == SOCKET_ERROR);
#else
    hasError = (::connect(socket, (sockaddr*)&clientAddr, sizeof(clientAddr)) == -1);
#endif

    if (hasError) {
        net_error("Error connecting to server");
        return false;
    }

    return true;
}

bool net_accept(const NetSocket& socket, NetSocket& client, std::string& ip, int& port)
{
    if (socket == InvalidSocket) return false;

    sockaddr_in clientAddr;
    socklen_t clientAddrSize = sizeof(clientAddr);

#ifdef _WIN32
    client = accept(socket, (sockaddr*)&clientAddr, (int*)&clientAddrSize);
    if (client == INVALID_SOCKET) {
#else
    client = accept(socket, (sockaddr*)&clientAddr, &clientAddrSize);
    if (client == -1) {
#endif
        ip = "";
        port = -1;
        net_error("Failed to accept client connection");
        return false;
    }

    char saddr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(clientAddr.sin_addr), saddr, INET_ADDRSTRLEN);
    ip = std::string(saddr);
    port = ntohs(clientAddr.sin_port);

    return true;
}

int net_send(const NetSocket& socket, char* data, int length, int flags)
{
    if (socket == InvalidSocket) return -1;

    int bytesSent = ::send(socket, data, length, flags);

#ifdef _WIN32
    if (bytesSent == SOCKET_ERROR) {
#else
    if (bytesSent == -1) {
#endif
        net_error("Failed to send data");
        return -1;
    }

    return bytesSent;
}

int net_recv(const NetSocket& socket, char* data, int length, int flags)
{
    if (socket == InvalidSocket) return -1;

    int bytesRecv = recv(socket, data, length, flags);

#ifdef _WIN32
    if (bytesRecv == SOCKET_ERROR) {
#else
    if (bytesRecv == -1) {
#endif
        net_error("Failed to receive data");
        return -1;
    }

    return bytesRecv;
}

int net_sendto(const NetSocket& socket, char* data, int length, int flags, const std::string& ip, int port)
{
    if (socket == InvalidSocket) return -1;

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip.c_str(), &(addr.sin_addr)) != 1) {
        net_error("Invalid IP address format");
        return -1;
    }

    int bytesSent = ::sendto(socket, data, length, flags, (struct sockaddr*)&addr, sizeof(addr));

#ifdef _WIN32
    if (bytesSent == SOCKET_ERROR) {
#else
    if (bytesSent == -1) {
#endif
        net_error("Failed to send data");
        return -1;
    }

    return bytesSent;
}

int net_recvfrom(const NetSocket& socket, char* data, int length, int flags, std::string& ip, int& port)
{
    if (socket == InvalidSocket) return -1;

    sockaddr_in addr;
    socklen_t addrSize = sizeof(struct sockaddr_in);
    memset(&addr, 0, sizeof(addr));

    int bytesRecv = ::recvfrom(socket, data, length, flags, (struct sockaddr*)&addr, &addrSize);

#ifdef _WIN32
    if (bytesRecv == SOCKET_ERROR) {
#else
    if (bytesRecv == -1) {
#endif
        net_error("Failed to receive data");
        return -1;
    }

    char saddr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(addr.sin_addr), saddr, INET_ADDRSTRLEN);
    ip = std::string(saddr);
    port = ntohs(addr.sin_port);

    return bytesRecv;
}
