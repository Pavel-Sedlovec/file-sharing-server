#include "protocol/ftp_handler.hpp"
#include "business/ftp_service.hpp"

#include <algorithm>
#include <cctype>

namespace {
std::string upper(std::string value) {
    for (char& c : value) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return value;
}
}

FtpHandler::FtpHandler() : service_(new FtpService()) {}
FtpHandler::~FtpHandler() { delete service_; }

void FtpHandler::onConnect(IConnection& conn) {
    state_ = SessionState::AUTHORIZATION;
    sendResponse(conn, 220, "Simple FTP server ready");
}

bool FtpHandler::onData(const char* data, size_t len, IConnection& conn) {
    if (!buffer_.append(data, len)) {
        sendResponse(conn, 500, "Line too long");
        return false;
    }
    while (auto command = buffer_.pop_command()) {
        if (!handleCommand(*command, conn)) return false;
    }
    return true;
}

void FtpHandler::onDisconnect() {}

void FtpHandler::sendResponse(IConnection& conn, int code, const std::string& text) {
    const std::string response = std::to_string(code) + " " + text + "\r\n";
    conn.send(response.c_str(), response.size());
}

bool FtpHandler::loggedIn(IConnection& conn) {
    if (state_ == SessionState::TRANSACTION) return true;
    sendResponse(conn, 530, "Please login with USER and PASS");
    return false;
}

bool FtpHandler::handleCommand(const Command& cmd, IConnection& conn) {
    const std::string name = upper(cmd.name);
    if (name == "USER") return handleUser(cmd.arg, conn);
    if (name == "PASS") return handlePass(cmd.arg, conn);
    if (name == "PWD") return handlePwd(conn);
    if (name == "TYPE") return handleType(cmd.arg, conn);
    if (name == "SITE") return handleSite(cmd.arg, conn);
    if (name == "PASV") return handlePasv(conn);
    if (name == "STOR") return handleStor(cmd.arg, conn);
    if (name == "RETR") return handleRetr(cmd.arg, conn);
    if (name == "DELE") return handleDele(cmd.arg, conn);
    if (name == "LIST") return handleList(conn);
    if (name == "QUIT") return handleQuit(conn);

    if (name == "CWD") return handleCwd(conn);
    if (name == "SYST") return handleSyst(conn);
    sendResponse(conn, 502, "Command not implemented: " + name);
    return true;
}
