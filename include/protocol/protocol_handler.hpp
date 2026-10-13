#ifndef PROTOCOL_HANDLER_HPP
#define PROTOCOL_HANDLER_HPP

#include <cstddef>
#include "protocol/connection_interfaces.hpp"

class IProtocolHandler {
public:
    virtual ~IProtocolHandler() = default;
    virtual void onConnect(IConnection& conn) = 0;
    virtual bool onData(const char* data, size_t len, IConnection& conn) = 0;
    virtual void onDisconnect() = 0;
};

#endif