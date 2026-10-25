#ifndef TCP_CONNECTION_HPP
#define TCP_CONNECTION_HPP

#include "protocol/connection_interfaces.hpp"
#include "server.hpp"

class tcp_connection : public IConnection {
private:
    int _fd;
    int passive_listen_fd_ = -1;
public:
    explicit tcp_connection(int fd);
    bool send(const char* data, std::size_t len) override;
    void close() override;
    std::optional<PassiveEndpoint> openPassiveDataChannel() override;
    bool hasPassiveDataChannel() const override;
    IDataConnection* acceptDataConnection() override;
    int get_fd() override;
    ~tcp_connection() override;
};

#endif
