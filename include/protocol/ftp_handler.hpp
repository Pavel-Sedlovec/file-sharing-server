#ifndef FTP_HANDLER_HPP
#define FTP_HANDLER_HPP

#include "protocol/protocol_handler.hpp"
#include "protocol/command_buffer.hpp"
#include <cstddef>
#include <string>

class FtpService;

enum class SessionState { NOT_CONNECTED, AUTHORIZATION, TRANSACTION, UPDATE };

class FtpHandler : public IProtocolHandler {
public:
    FtpHandler();
    void onConnect(IConnection& conn) override;
    bool onData(const char* data, size_t len, IConnection& conn) override;
    void onDisconnect() override;
    ~FtpHandler() override;

private:
    CommandBuffer buffer_;
    SessionState state_ = SessionState::NOT_CONNECTED;
    std::string pending_user_;
    std::string current_user_;
    FtpService* service_;

    bool handleCommand(const Command& cmd, IConnection& conn);
    void sendResponse(IConnection& conn, int code, const std::string& text);
    bool handleUser(const std::string& arg, IConnection& conn);
    bool handlePass(const std::string& arg, IConnection& conn);
    bool handlePwd(IConnection& conn);
    bool handleQuit(IConnection& conn);
    bool handleSite(const std::string& arg, IConnection& conn);
    bool handlePasv(IConnection& conn);
    bool handleStor(const std::string& arg, IConnection& conn);
    bool handleRetr(const std::string& arg, IConnection& conn);
    bool handleList(IConnection& conn);
    bool loggedIn(IConnection& conn);
};

#endif
