#include "protocol/ftp_handler.hpp"
#include "business/ftp_service.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace {
std::string upper(std::string value) {
    for (char& c : value)
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return value;
}
}

FtpHandler::FtpHandler() : service_(new FtpService()) {}

FtpHandler::~FtpHandler() {
    delete service_;
}

void FtpHandler::onConnect(IConnection& conn) {
    state_ = SessionState::AUTHORIZATION;
    sendResponse(conn, 220, "Simple FTP server ready");
}

bool FtpHandler::onData(const char* data, size_t len, IConnection& conn) {
    if (!buffer_.append(data, len)) {
        sendResponse(conn, 500, "Line too long");
        return false;
    }
    while (auto cmd = buffer_.pop_command()) {
        if (!handleCommand(*cmd, conn)) return false;
    }
    return true;
}

void FtpHandler::onDisconnect() {}

void FtpHandler::sendResponse(IConnection& conn, int code, const std::string& text) {
    const std::string message = std::to_string(code) + " " + text + "\r\n";
    conn.send(message.c_str(), message.size());
}

bool FtpHandler::loggedIn(IConnection& conn) {
    if (state_ != SessionState::TRANSACTION) {
        sendResponse(conn, 530, "Please login with USER and PASS");
        return false;
    }
    return true;
}

bool FtpHandler::handleCommand(const Command& cmd, IConnection& conn) {
    const std::string name = upper(cmd.name);
    if (name == "USER") return handleUser(cmd.arg, conn);
    if (name == "PASS") return handlePass(cmd.arg, conn);
    if (name == "PWD") return handlePwd(conn);
    if (name == "SITE") return handleSite(cmd.arg, conn);
    if (name == "PASV") return handlePasv(conn);
    if (name == "STOR") return handleStor(cmd.arg, conn);
    if (name == "RETR") return handleRetr(cmd.arg, conn);
    if (name == "LIST") return handleList(conn);
    if (name == "QUIT") return handleQuit(conn);
    sendResponse(conn, 502, "Command not implemented: " + name);
    return true;
}

bool FtpHandler::handleUser(const std::string& arg, IConnection& conn) {
    if (arg.empty()) {
        sendResponse(conn, 501, "USER requires a login");
        return true;
    }
    pending_user_ = arg;
    current_user_.clear();
    state_ = SessionState::AUTHORIZATION;
    sendResponse(conn, 331, "Username OK, need password");
    return true;
}

bool FtpHandler::handlePass(const std::string& arg, IConnection& conn) {
    if (pending_user_.empty()) {
        sendResponse(conn, 503, "Login with USER first");
        return true;
    }
    if (service_->authenticate(pending_user_, arg) && service_->prepareUserStorage(pending_user_)) {
        current_user_ = pending_user_;
        pending_user_.clear();
        state_ = SessionState::TRANSACTION;
        sendResponse(conn, 230, "User logged in");
    } else {
        pending_user_.clear();
        sendResponse(conn, 530, "Login incorrect");
    }
    return true;
}

bool FtpHandler::handlePwd(IConnection& conn) {
    if (!loggedIn(conn)) return true;
    sendResponse(conn, 257, "\"/\" is the current directory");
    return true;
}

bool FtpHandler::handleSite(const std::string& arg, IConnection& conn) {
    std::istringstream input(arg);
    std::string subcommand, folder, password;
    input >> subcommand >> folder >> password;
    if (upper(subcommand) != "REGISTER" || folder.empty() || password.empty()) {
        sendResponse(conn, 501, "Usage: SITE REGISTER <folder> <password>");
        return true;
    }
    const std::string login = service_->registerUser(folder, password);
    if (login.empty()) {
        sendResponse(conn, 550, "Cannot register user or invalid folder");
        return true;
    }
    sendResponse(conn, 200, "Registered. Login: " + login);
    return true;
}

bool FtpHandler::handlePasv(IConnection& conn) {
    if (!loggedIn(conn)) return true;
    const auto endpoint = conn.openPassiveDataChannel();
    if (!endpoint) {
        sendResponse(conn, 425, "Cannot open passive connection");
        return true;
    }

    // FTP PASV response encodes IPv4 address and port as six comma-separated numbers.
    unsigned int a = 127, b = 0, c = 0, d = 1;
    std::istringstream ip(endpoint->ip);
    char dot1 = 0, dot2 = 0, dot3 = 0;
    if (!(ip >> a >> dot1 >> b >> dot2 >> c >> dot3 >> d) ||
        dot1 != '.' || dot2 != '.' || dot3 != '.' ||
        a > 255 || b > 255 || c > 255 || d > 255) {
        sendResponse(conn, 425, "Invalid passive address");
        return true;
    }
    const unsigned int p1 = endpoint->port / 256;
    const unsigned int p2 = endpoint->port % 256;
    sendResponse(conn, 227, "Entering Passive Mode (" + std::to_string(a) + "," +
                 std::to_string(b) + "," + std::to_string(c) + "," + std::to_string(d) + "," +
                 std::to_string(p1) + "," + std::to_string(p2) + ")");
    return true;
}

bool FtpHandler::handleStor(const std::string& arg, IConnection& conn) {
    if (!loggedIn(conn)) return true;
    if (arg.empty()) {
        sendResponse(conn, 501, "STOR requires filename");
        return true;
    }
    if (!conn.hasPassiveDataChannel()) {
        sendResponse(conn, 425, "Use PASV first");
        return true;
    }
    sendResponse(conn, 150, "Opening data connection for STOR");
    IDataConnection* data = conn.acceptDataConnection();
    if (!data) {
        sendResponse(conn, 425, "Data connection failed");
        return true;
    }
    const bool success = service_->storeFile(current_user_, arg, *data);
    data->close();
    delete data;
    sendResponse(conn, success ? 226 : 550,
                 success ? "Transfer complete" : "Cannot store file or data transfer failed");
    return true;
}

bool FtpHandler::handleRetr(const std::string& arg, IConnection& conn) {
    if (!loggedIn(conn)) return true;
    if (arg.empty()) {
        sendResponse(conn, 501, "RETR requires filename");
        return true;
    }
    if (!conn.hasPassiveDataChannel()) {
        sendResponse(conn, 425, "Use PASV first");
        return true;
    }
    sendResponse(conn, 150, "Opening data connection for RETR");
    IDataConnection* data = conn.acceptDataConnection();
    if (!data) {
        sendResponse(conn, 425, "Data connection failed");
        return true;
    }
    const bool success = service_->retrieveFile(current_user_, arg, *data);
    data->close();
    delete data;
    sendResponse(conn, success ? 226 : 550,
                 success ? "Transfer complete" : "File not found or transfer failed");
    return true;
}

bool FtpHandler::handleList(IConnection& conn) {
    if (!loggedIn(conn)) return true;
    if (!conn.hasPassiveDataChannel()) {
        sendResponse(conn, 425, "Use PASV first");
        return true;
    }
    sendResponse(conn, 150, "Opening data connection for LIST");
    IDataConnection* data = conn.acceptDataConnection();
    if (!data) {
        sendResponse(conn, 425, "Data connection failed");
        return true;
    }
    const bool success = service_->sendDirectoryListing(current_user_, *data);
    data->close();
    delete data;
    sendResponse(conn, success ? 226 : 550,
                 success ? "Directory send complete" : "Cannot list directory");
    return true;
}

bool FtpHandler::handleQuit(IConnection& conn) {
    state_ = SessionState::UPDATE;
    sendResponse(conn, 221, "Goodbye");
    return false;
}
