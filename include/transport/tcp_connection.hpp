#ifndef TCP_CONNECTION_HPP
#define TCP_CONNECTION_HPP

#include "protocol/protocol_handler.hpp"
#include "protocol/connection_interfaces.hpp"
#include "server.hpp"

class tcp_connection : public IConnection
{
private:
    int _fd;
public:

    tcp_connection(int fd);
    bool send(const char* data, std::size_t len) override;
    void close() override;
    std::optional<PassiveEndpoint> openPassiveDataChannel() override;
    IDataConnection* acceptDataConnection() override;

    int get_fd() override;

    ~tcp_connection() override;

private:
    int fd_;
    int passive_listen_fd_ = -1;
};

#endif