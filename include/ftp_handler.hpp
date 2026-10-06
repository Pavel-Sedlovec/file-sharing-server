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

    ~FtpHandler() override;
};

#endif