#ifndef CONNECTION_INTERFACES_HPP
#define CONNECTION_INTERFACES_HPP

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

enum class IoStatus { OK, END_OF_STREAM, ERROR };

struct IoResult {
    IoStatus status;
    std::size_t bytes;
};

struct PassiveEndpoint {
    std::string ip;
    std::uint16_t port = 0;
};

class IDataConnection {
public:
    virtual ~IDataConnection() = default;
    virtual IoResult receive(char* buffer, std::size_t capacity) = 0;
    virtual IoResult send(const char* buffer, std::size_t size) = 0;
    virtual void close() = 0;
};

class IConnection {
public:
    virtual ~IConnection() = default;
    virtual bool send(const char* data, std::size_t len) = 0;
    virtual void close() = 0;
    virtual int get_fd() = 0;
    virtual std::optional<PassiveEndpoint> openPassiveDataChannel() = 0;
    virtual bool hasPassiveDataChannel() const = 0;
    virtual IDataConnection* acceptDataConnection() = 0;
};

#endif
