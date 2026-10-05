#ifndef FTP_HANDLER_HPP
#define FTP_HANDLER_HPP

#include "protocol_handler.hpp"
#include <cstddef>
#include <string>
#include <optional>

class FtpHandler : public IProtocolHandler{
public:
    void onConnect(IConnection& conn) override;
    bool onData(const char* data, size_t len, IConnection& conn) override;
    void onDisconnect() override;

    ~ftp_protocol() override;
};

struct Command
{
    std::string name;
    std::string arg;
};


class CommandBuffer{
private:
    char** _buffer;
    int _size;
public:
    CommandBuffer(int size = 2048);
    void resize();
    bool check_resize();
    void set_data(char* data, size_t len);
    Command get_data();
};

#endif