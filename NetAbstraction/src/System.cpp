#include "netAbstraction/System.h"

#include <iostream>

std::vector<std::string> GetAdapterList()
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
        const std::wstring& wstr = adapter->FriendlyName;
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, NULL, 0, NULL, NULL);
        std::string adapterName(size_needed - 1, 0);
        WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &adapterName[0], size_needed - 1, NULL, NULL);
        result.push_back(adapterName);
    }
    free(pAdapterAddresses);
    return result;
}

std::string GetAdapterAddress(const std::string& adapterName)
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
}

std::string ComputeBroadcastAddress(const std::string& ipStr)
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

std::string GetBroadcastAddress(const std::string& interfaceName) {
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
}

int net_select(const NetSocket& socket, bool& readSet)
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

int net_select(const std::vector<NetSocket>& sockets, std::vector<bool>& readSet)
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

    readSet = std::vector<bool>(maxFd + 1, false);  // Boolean array indexed by FD values
    for (int sock = 0; sock <= maxFd; ++sock) {
        if (FD_ISSET(sock, &rs)) {
            readSet[sock] = true;
        }
    }

    return selectResult;
}


bool net_initialize()
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed: " << WSAGetLastError() << std::endl;
        return false;
    }
    return true;
}

void net_release()
{
    WSACleanup();
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
        std::cerr << "Unknown protocol" << std::endl;
        return false;
    }
    if (sock == INVALID_SOCKET) {
        std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl;
        return false;
    }
    return true;
}

void net_close(NetSocket& socket)
{
    if (socket == InvalidSocket) return;
    closesocket(socket);
    socket = InvalidSocket;
}

bool net_reuse(const NetSocket& socket, bool reuse)
{
    if (socket == InvalidSocket) return false;
    int _reuse = reuse ? 1 : 0;
    if (setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, (char*)&_reuse, sizeof(_reuse)) == SOCKET_ERROR) {
        std::cerr << "setsockopt(SO_REUSEADDR) failed!" << std::endl;
        return false;
    }
    return true;
}

bool net_nonblocking(const NetSocket& socket, bool nonblock)
{
    if (socket == InvalidSocket) return false;
    u_long mode = nonblock ? 1 : 0;  // 1 = Non-blocking
    if (ioctlsocket(socket, FIONBIO, &mode) != 0) {
        std::cerr << "Error: Failed to set non-blocking mode, code: " << WSAGetLastError() << std::endl;
        return false;
    }
    return true;
}

bool net_broadcast(const NetSocket& socket, bool bcast)
{
    if (socket == InvalidSocket) return false;
    // Enable SO_BROADCAST to receive broadcast messages
    int broadcastEnable = bcast ? 1 : 0;
    if (setsockopt(socket, SOL_SOCKET, SO_BROADCAST, (char*)&broadcastEnable, sizeof(broadcastEnable)) == SOCKET_ERROR) {
        std::cerr << "setsockopt(SO_BROADCAST) failed!" << std::endl;
        return false;
    }
    return true;
}

bool net_bindany(const NetSocket& socket, int port)
{
    if (socket == InvalidSocket) return false;
    sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    if (bind(socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << "Binding failed!" << std::endl;
        return false;
    }
    return true;
}

bool net_bindintf(const NetSocket& socket, const std::string& intf, int port)
{
    if (socket == InvalidSocket) return false;
    if (intf == "")
    {
        std::cerr << "No interface provided" << std::endl;
        return false;
    }
    sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    std::string ip = GetAdapterAddress(intf);
    if (ip == "")
    {
        std::cerr << "local ip address of interface not found" << std::endl;
        return false;
    }
    inet_pton(AF_INET, ip.c_str(), &(server_addr.sin_addr));
    if (bind(socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << "Binding failed!" << std::endl;
        return false;
    }
    return true;
}

bool net_bindip(const NetSocket& socket, const std::string& ip, int port)
{
    if (socket == InvalidSocket) return false;
    if (ip == "")
    {
        std::cerr << "No IP address provided" << std::endl;
        return false;
    }
    sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &(server_addr.sin_addr));
    if (bind(socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << "Binding failed!" << std::endl;
        return false;
    }
    return true;
}

bool net_listen(const NetSocket& socket)
{
    if (socket == InvalidSocket) return false;
    if (listen(socket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed: " << WSAGetLastError() << std::endl;
        return false;
    }
    return true;
}

bool net_connect(const NetSocket& socket, const std::string& ip, int port)
{
    if (socket == InvalidSocket) return false;
    sockaddr_in clientAddr;
    clientAddr.sin_family = AF_INET;
    clientAddr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &(clientAddr.sin_addr));
    if (::connect(socket, (sockaddr*)&clientAddr, sizeof(clientAddr)) == SOCKET_ERROR) {
        std::cerr << "Error connecting to server: " << WSAGetLastError() << std::endl;
        return false;
    }
    return true;
}

bool net_accept(const NetSocket& socket, NetSocket& client, std::string& ip, int& port)
{
    if (socket == InvalidSocket) return false;
    sockaddr_in clientAddr;
    int clientAddrSize = sizeof(clientAddr);
    client = accept(socket, (sockaddr*)&clientAddr, &clientAddrSize);
    if (client == INVALID_SOCKET)
    {
        ip = "";
        port = -1;
        return false;
    }
    char saddr[20];
    inet_ntop(AF_INET, &(clientAddr.sin_addr), saddr, 20);
    ip = std::string(saddr);
    port = ntohs(clientAddr.sin_port);
    return true;
}

int net_send(const NetSocket& socket, char* data, int length, int flags)
{
    int bytesSent = ::send(socket, data, length, flags);
    if (bytesSent == SOCKET_ERROR) {
        std::cerr << "Failed to send data to client: " << WSAGetLastError() << std::endl;
        return -1;
    }
    return bytesSent;
}

int net_recv(const NetSocket& socket, char* data, int length, int flags)
{
    int bytesRecv = recv(socket,data, length, 0);
    if (bytesRecv <= 0) {
        std::cerr << "Failed to receive data from client: " << WSAGetLastError() << std::endl;
        return -1;
    }
    return bytesRecv;
}

int net_sendto(const NetSocket& socket, char* data, int length, int flags, const std::string& ip, int port)
{
    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &(addr.sin_addr));
    int bytesSent = ::sendto(socket, data, length, flags, (struct sockaddr*) & addr, sizeof(addr));
    if (bytesSent == SOCKET_ERROR) {
        std::cerr << "Failed to send data to client: " << WSAGetLastError() << std::endl;
        return -1;
    }
    return bytesSent;
}

int net_recvfrom(const NetSocket& socket, char* data, int length, int flags, std::string& ip, int& port)
{
    sockaddr_in addr;
    int sz = sizeof(struct sockaddr_in);
    memset(&addr, 0, sizeof(addr));
    int bytesRecv = ::recvfrom(socket, data, length, flags, (struct sockaddr*)&addr, &sz);
    if (bytesRecv == SOCKET_ERROR) {
        std::cerr << "Failed to recv data from client: " << WSAGetLastError() << std::endl;
        return -1;
    }
    char saddr[20];
    inet_ntop(AF_INET, &(addr.sin_addr), saddr, 20);
    ip = std::string(saddr);
    port = ntohs(addr.sin_port);
    return bytesRecv;
}