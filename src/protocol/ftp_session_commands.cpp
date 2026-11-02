#include "protocol/ftp_handler.hpp"
#include "business/ftp_service.hpp"

#include <sstream>
#include <cctype>

namespace {
std::string upperCommand(std::string value) {
    for (char& c : value) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return value;
}
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

bool FtpHandler::handleType(const std::string& arg, IConnection& conn) {
    std::istringstream input(arg);
    std::string subcommand;
    input >> subcommand;
    if (upperCommand(subcommand) != "A" && upperCommand(subcommand) != "I") {
        sendResponse(conn, 501, "Usage: TYPE <subcommand>");
        return true;
    }
    sendResponse(conn, 200, "Type set to OK");
    return true;
}

bool FtpHandler::handleSite(const std::string& arg, IConnection& conn) {
    std::istringstream input(arg);
    std::string subcommand, username, password;
    input >> subcommand >> username >> password;
    if (upperCommand(subcommand) != "REGISTER" || username.empty() || password.empty()) {
        sendResponse(conn, 501, "Usage: SITE REGISTER <username> <password>");
        return true;
    }
    const std::string registered = service_->registerUser(username, password);
    
    if (registered.empty()) {
        sendResponse(conn, 550, "Registration failed");
        return true;
    }
    sendResponse(conn, 200, "Registered. Login: " + registered);
    return true;
}

bool FtpHandler::handleQuit(IConnection& conn) {
    state_ = SessionState::UPDATE;
    sendResponse(conn, 221, "Goodbye");
    return false;
}


bool FtpHandler::handleCwd(IConnection& conn) {
    sendResponse(conn, 250, "Directory successfully changed.");
    return true;
}

bool FtpHandler::handleSyst(IConnection& conn) {
    sendResponse(conn, 215, "UNIX Type: L8");
    return true;
}