#ifndef PROTOCOL_HANDLER_HPP
#define PROTOCOL_HANDLER_HPP

#include <cstddef>

class IConnection {
public:
    virtual ~IConnection() = default;
    virtual bool send(const char* data, size_t len) = 0;
    virtual void close() = 0;
    virtual int get_fd() = 0;
};

class IProtocolHandler {
public:
    virtual ~IProtocolHandler() = default;
    virtual void onConnect(IConnection& conn) = 0;
    virtual bool onData(const char* data, size_t len, IConnection& conn) = 0;
    virtual void onDisconnect() = 0;
};

#endif