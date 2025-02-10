#pragma once

#include <mutex>
#include <atomic>
#include <string>

#include "netAbstraction/Layers.h"

class NetProcess
{
public:
protected:
    NetProcess(const std::string& intf = "");
    ~NetProcess();

    Layer::Socket sock;
    std::mutex clientMutex;
    std::atomic<bool> startedFlag;
    std::atomic<bool> isConnected;
    std::atomic<bool> stopFlag;
    std::atomic<int> threadsCompleted;
    std::string intfToUse;
};
