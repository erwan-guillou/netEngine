#include "netAbstraction/NetProcess.h"

NetProcess::NetProcess(const std::string& intf)
    : sock(InvalidSocket)
    , startedFlag(false)
    , isConnected(false)
    , stopFlag(false)
    , threadsCompleted(0)
    , intfToUse(intf)
{
    Layer::initialize();
}

NetProcess::~NetProcess()
{
    Layer::release();
}
