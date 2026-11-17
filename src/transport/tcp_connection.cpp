#include "transport/tcp_connection.hpp"
#include <arpa/inet.h>
#include <cerrno>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

namespace {
class TcpDataConnection : public IDataConnection {
public:
    explicit TcpDataConnection(int fd) : fd_(fd) {}
    ~TcpDataConnection() override { close(); }

    IoResult receive(char* buffer, std::size_t capacity) override {
        while (true) {
            ssize_t n = ::recv(fd_, buffer, capacity, 0);
            if (n > 0) return {IoStatus::OK, static_cast<std::size_t>(n)};
            if (n == 0) return {IoStatus::END_OF_STREAM, 0};
            if (errno == EINTR) continue;
            return {IoStatus::ERROR, 0};
        }
    }

    IoResult send(const char* buffer, std::size_t size) override {
        std::size_t total = 0;
        while (total < size) {
            ssize_t n = ::send(fd_, buffer + total, size - total, 0);
            if (n < 0 && errno == EINTR) continue;
            if (n <= 0) return {IoStatus::ERROR, total};
            total += static_cast<std::size_t>(n);
        }
        return {IoStatus::OK, total};
    }

    void close() override {
        if (fd_ >= 0) {
            ::close(fd_);
            fd_ = -1;
        }
    }
private:
    int fd_;
};
}

tcp_connection::tcp_connection(int fd) : _fd(fd) {}

bool tcp_connection::send(const char* data, std::size_t len) {
    return send_all(_fd, data, len);
}

void tcp_connection::close() {
    if (_fd >= 0) {
        ::close(_fd);
        _fd = -1;
    }
    if (passive_listen_fd_ >= 0) {
        ::close(passive_listen_fd_);
        passive_listen_fd_ = -1;
    }
}

int tcp_connection::get_fd() { return _fd; }

std::optional<PassiveEndpoint> tcp_connection::openPassiveDataChannel() {
    if (passive_listen_fd_ >= 0) {
        ::close(passive_listen_fd_);
        passive_listen_fd_ = -1;
    }

    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return std::nullopt;
    int opt = 1;
    ::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = 0;
    if (::bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0 ||
        ::listen(fd, 1) < 0) {
        ::close(fd);
        return std::nullopt;
    }

    socklen_t len = sizeof(addr);
    if (::getsockname(fd, reinterpret_cast<sockaddr*>(&addr), &len) < 0) {
        ::close(fd);
        return std::nullopt;
    }
    passive_listen_fd_ = fd;
    return PassiveEndpoint{"172.23.196.62", ntohs(addr.sin_port)};
}

bool tcp_connection::hasPassiveDataChannel() const {
    return passive_listen_fd_ >= 0;
}

IDataConnection* tcp_connection::acceptDataConnection() {
    if (passive_listen_fd_ < 0) return nullptr;
    int listen_fd = passive_listen_fd_;
    passive_listen_fd_ = -1;
    int data_fd;
    do {
        data_fd = ::accept(listen_fd, nullptr, nullptr);
    } while (data_fd < 0 && errno == EINTR);
    ::close(listen_fd);
    if (data_fd < 0) return nullptr;
    return new TcpDataConnection(data_fd);
}

tcp_connection::~tcp_connection() { close(); }
