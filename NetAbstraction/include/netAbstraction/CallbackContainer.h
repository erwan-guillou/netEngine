#pragma once

#include <vector>

template <typename cbDefs>
class CallbackContainer
{
public:
    CallbackContainer()
        : _cbReceiveNextID(0)
        , _cbConnectNextID(0)
        , _cbDisconnectNextID(0)
        , _cbHandshake(nullptr)
    {
    }
    size_t addReceiver(typename cbDefs::ReceiverCallback cb)
    {
        size_t id = _cbReceiveNextID++;
        _cbReceive.push_back({ id, cb });
        return id;
    }
    void removeReceiver(size_t id)
    {
        auto it = std::remove_if(_cbReceive.begin(), _cbReceive.end(),
            [id](const std::pair<size_t, typename cbDefs::ReceiverCallback>& pair) {
                return pair.first == id;
            });
        _cbReceive.erase(it, _cbReceive.end());
    }

    size_t addConnector(typename cbDefs::ConnectedCallback cb)
    {
        size_t id = _cbConnectNextID++;
        _cbConnect.push_back({ id, cb });
        return id;
    }
    void removeConnector(size_t id)
    {
        auto it = std::remove_if(_cbConnect.begin(), _cbConnect.end(),
            [id](const std::pair<size_t, typename cbDefs::ConnectedCallback>& pair) {
                return pair.first == id;
            });
        _cbConnect.erase(it, _cbConnect.end());
    }

    size_t addDisconnector(typename cbDefs::DisconnectedCallback cb)
    {
        size_t id = _cbDisconnectNextID++;
        _cbDisconnect.push_back({ id, cb });
        return id;
    }
    void removeDisconnector(size_t id)
    {
        auto it = std::remove_if(_cbDisconnect.begin(), _cbDisconnect.end(),
            [id](const std::pair<size_t, typename cbDefs::DisconnectedCallback>& pair) {
                return pair.first == id;
            });
        _cbDisconnect.erase(it, _cbDisconnect.end());
    }

    void setHandshake(typename cbDefs::HandshakingCallback cb)
    {
        _cbHandshake = cb;
    }

protected:
    std::vector<std::pair<size_t, typename cbDefs::ReceiverCallback>> _cbReceive;
    size_t _cbReceiveNextID;
    std::vector<std::pair<size_t, typename cbDefs::ConnectedCallback>> _cbConnect;
    size_t _cbConnectNextID;
    std::vector<std::pair<size_t, typename cbDefs::DisconnectedCallback>> _cbDisconnect;
    size_t _cbDisconnectNextID;
    typename cbDefs::HandshakingCallback _cbHandshake;
};
