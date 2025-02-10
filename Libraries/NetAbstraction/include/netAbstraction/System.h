#pragma once

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <conio.h>
#include <winsock2.h>
#include <iphlpapi.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")
using NetSocket = SOCKET;
const NetSocket InvalidSocket = INVALID_SOCKET;

#else // not _WIN32

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ifaddrs.h>
#include <netdb.h>
using NetSocket = int;
const NetSocket InvalidSocket = -1;

#endif // _WIN32

#include <string>
#include <vector>

unsigned int MaximumPacketSize(NetSocket sock);
//const unsigned int MaximumPayload = MaximumPacketSize - sizeof(unsigned int);

const unsigned char NetUDP = 0;
const unsigned char NetTCP = 1;

std::vector<std::string> GetAdapterList();
std::string GetAdapterAddress(const std::string& adapterName);
std::string ComputeBroadcastAddress(const std::string& ipStr);
std::string GetBroadcastAddress(const std::string& interfaceName);

int net_select(const NetSocket& socket, bool& readSet);
int net_select(const std::vector<NetSocket>& sockets, std::vector<bool>& readSet);

bool net_initialize();
void net_release();

bool net_open(NetSocket& socket, unsigned char proto = NetTCP);
void net_close(NetSocket& socket);

bool net_reuse(const NetSocket& socket, bool reuse = true);
bool net_nonblocking(const NetSocket& socket, bool nonblock = true);
bool net_broadcast(const NetSocket& socket, bool bcast = true);

bool net_bindany(const NetSocket& socket, int port);
bool net_bindintf(const NetSocket& socket, const std::string& intf, int port);
bool net_bindip(const NetSocket& socket, const std::string& ip, int port);

bool net_listen(const NetSocket& socket);

bool net_connect(const NetSocket& socket, const std::string& ip, int port);

bool net_accept(const NetSocket& socket, NetSocket& client, std::string& ip, int& port);

int net_send(const NetSocket& socket, char* data, int length, int flags);
int net_recv(const NetSocket& socket, char* data, int length, int flags);

int net_sendto(const NetSocket& socket, char* data, int length, int flags, const std::string& ip, int port);
int net_recvfrom(const NetSocket& socket, char* data, int length, int flags, std::string& ip, int& port);
