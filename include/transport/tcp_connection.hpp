#ifndef TCP_CONNECTION_HPP
#define TCP_CONNECTION_HPP

#include "protocol/protocol_handler.hpp"
#include "server.hpp"

class tcp_connection : public IConnection
{
private:
    int _fd;
public:

    tcp_connection(int fd);
    bool send(const char* data, size_t len) override;
    void close() override;

    int get_fd() override;

    ~tcp_connection() override;
};

#endif