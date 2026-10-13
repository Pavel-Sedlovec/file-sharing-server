#include "protocol/ftp_handler.hpp"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <cerrno>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <random>
#include <sstream>
#include <unordered_map>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

namespace fs = std::filesystem;

namespace {
struct UserRecord {
    std::string password;
    std::string folder;
};

std::mutex users_mutex;
std::unordered_map<std::string, UserRecord> users = {
    {"admin", {"1234", "admin"}}
};
int next_user_id = 1001;

std::string upper(std::string s) {
    for (char& c : s)
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return s;
}
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
        if (!handleCommand(*cmd, conn))
            return false;
    }
    return true;
}

void FtpHandler::onDisconnect() {
    closeDataSocket();
}

FtpHandler::~FtpHandler() {
    closeDataSocket();
}

void FtpHandler::sendResponse(IConnection& conn, int code, const std::string& text) {
    std::string msg = std::to_string(code) + " " + text + "\r\n";
    conn.send(msg.c_str(), msg.size());
}

bool FtpHandler::checkCredentials(const std::string& user, const std::string& pass) const {
    std::lock_guard<std::mutex> lock(users_mutex);
    auto it = users.find(user);
    return it != users.end() && it->second.password == pass;
}

std::string FtpHandler::userRoot() const {
    std::lock_guard<std::mutex> lock(users_mutex);
    auto it = users.find(current_user_);
    if (it == users.end()) return {};
    return "storage/" + it->second.folder;
}

bool FtpHandler::loggedIn(IConnection& conn) {
    if (state_ != SessionState::TRANSACTION) {
        sendResponse(conn, 530, "Please login with USER and PASS");
        return false;
    }
    return true;
}

void FtpHandler::closeDataSocket() {
    if (data_listen_fd_ >= 0) {
        ::close(data_listen_fd_);
        data_listen_fd_ = -1;
    }
}

bool FtpHandler::handleCommand(const Command& cmd, IConnection& conn) {
    std::string name = upper(cmd.name);
    if (name == "USER") return handleUser(cmd.arg, conn);
    if (name == "PASS") return handlePass(cmd.arg, conn);
    if (name == "PWD")  return handlePwd(conn);
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
    if (checkCredentials(pending_user_, arg)) {
        current_user_ = pending_user_;
        pending_user_.clear();
        state_ = SessionState::TRANSACTION;
        fs::create_directories(userRoot());
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
    std::istringstream iss(arg);
    std::string subcommand, folder, password;
    iss >> subcommand >> folder >> password;

    if (upper(subcommand) != "REGISTER" || folder.empty() || password.empty()) {
        sendResponse(conn, 501, "Usage: SITE REGISTER <folder> <password>");
        return true;
    }

    std::string login;
    {
        std::lock_guard<std::mutex> lock(users_mutex);
        do {
            login = "user" + std::to_string(next_user_id++);
        } while (users.count(login));
        users[login] = {password, folder};
    }

    fs::create_directories("storage/" + folder);
    sendResponse(conn, 200, "Registered. Login: " + login);
    return true;
}

bool FtpHandler::handlePasv(IConnection& conn) {
    if (!loggedIn(conn)) return true;

    closeDataSocket();
    data_listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (data_listen_fd_ < 0) {
        sendResponse(conn, 425, "Cannot open passive connection");
        return true;
    }

    int opt = 1;
    setsockopt(data_listen_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0;

    if (bind(data_listen_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0 ||
        listen(data_listen_fd_, 1) < 0) {
        closeDataSocket();
        sendResponse(conn, 425, "Cannot open passive connection");
        return true;
    }

    socklen_t len = sizeof(addr);
    getsockname(data_listen_fd_, reinterpret_cast<sockaddr*>(&addr), &len);
    int port = ntohs(addr.sin_port);

    int p1 = port / 256;
    int p2 = port % 256;
    std::cerr << "[DEBUG][PASV] listening fd=" << data_listen_fd_
              << " port=" << port << " (" << p1 << "," << p2 << ")" << std::endl;

    sendResponse(conn, 227, "Entering Passive Mode (127,0,0,1," +
                              std::to_string(p1) + "," + std::to_string(p2) + ")");
    return true;
}

bool FtpHandler::handleStor(const std::string& arg, IConnection& conn) {
    if (!loggedIn(conn)) return true;
    if (arg.empty()) {
        sendResponse(conn, 501, "STOR requires filename");
        return true;
    }
    if (data_listen_fd_ < 0) {
        sendResponse(conn, 425, "Use PASV first");
        return true;
    }

    fs::path root = fs::weakly_canonical(userRoot());
    fs::path target = fs::weakly_canonical(root / fs::path(arg).filename());
    if (target.parent_path() != root) {
        std::cerr << "[DEBUG][STOR] invalid target: root=" << root
                  << " target=" << target << std::endl;
        sendResponse(conn, 550, "Invalid filename");
        return true;
    }

    std::cerr << "[DEBUG][STOR] requested file: " << arg << std::endl;
    std::cerr << "[DEBUG][STOR] root: " << root << std::endl;
    std::cerr << "[DEBUG][STOR] target: " << target << std::endl;
    std::cerr << "[DEBUG][STOR] listening fd before accept: "
              << data_listen_fd_ << std::endl;

    sendResponse(conn, 150, "Opening data connection for STOR");
    std::cerr << "[DEBUG][STOR] sent 150, waiting for data connection..." << std::endl;

    int data_fd = accept(data_listen_fd_, nullptr, nullptr);
    std::cerr << "[DEBUG][STOR] accept() returned fd=" << data_fd
              << " errno=" << errno << " (" << std::strerror(errno) << ")" << std::endl;

    closeDataSocket();

    if (data_fd < 0) {
        sendResponse(conn, 425, "Data connection failed");
        return true;
    }

    std::cerr << "[DEBUG][STOR] data connection accepted, fd=" << data_fd << std::endl;

    std::ofstream out(target, std::ios::binary);
    if (!out) {
        std::cerr << "[DEBUG][STOR] cannot open output file, errno=" << errno
                  << " (" << std::strerror(errno) << ")" << std::endl;
        ::close(data_fd);
        sendResponse(conn, 550, "Cannot create file");
        return true;
    }

    std::cerr << "[DEBUG][STOR] output file opened successfully" << std::endl;

    char buf[8192];
    ssize_t n;
    std::size_t total_bytes = 0;
    int recv_calls = 0;

    while (true) {
        errno = 0;
        n = recv(data_fd, buf, sizeof(buf), 0);
        ++recv_calls;

        std::cerr << "[DEBUG][STOR] recv #" << recv_calls
                  << ": n=" << n
                  << " errno=" << errno;
        if (n < 0)
            std::cerr << " (" << std::strerror(errno) << ")";
        std::cerr << std::endl;

        if (n > 0) {
            out.write(buf, n);
            total_bytes += static_cast<std::size_t>(n);

            if (!out) {
                std::cerr << "[DEBUG][STOR] file write failed after "
                          << total_bytes << " bytes" << std::endl;
                break;
            }

            std::cerr << "[DEBUG][STOR] wrote " << n
                      << " bytes, total=" << total_bytes << std::endl;
        } else {
            break;
        }
    }

    out.close();
    ::close(data_fd);

    std::cerr << "[DEBUG][STOR] transfer finished, total bytes="
              << total_bytes << std::endl;

    if (n < 0) {
        sendResponse(conn, 426, "Data transfer error");
        return true;
    }

    sendResponse(conn, 226, "Transfer complete");
    return true;
}

bool FtpHandler::handleRetr(const std::string& arg, IConnection& conn) {
    if (!loggedIn(conn)) return true;
    if (arg.empty()) {
        sendResponse(conn, 501, "RETR requires filename");
        return true;
    }
    if (data_listen_fd_ < 0) {
        sendResponse(conn, 425, "Use PASV first");
        return true;
    }

    fs::path root = fs::weakly_canonical(userRoot());
    fs::path target = fs::weakly_canonical(root / fs::path(arg).filename());
    if (target.parent_path() != root || !fs::is_regular_file(target)) {
        sendResponse(conn, 550, "File not found");
        return true;
    }

    sendResponse(conn, 150, "Opening data connection for RETR");

    int data_fd = accept(data_listen_fd_, nullptr, nullptr);
    closeDataSocket();
    if (data_fd < 0) {
        sendResponse(conn, 425, "Data connection failed");
        return true;
    }

    std::ifstream in(target, std::ios::binary);
    char buf[8192];
    while (in.read(buf, sizeof(buf)) || in.gcount() > 0) {
        std::streamsize n = in.gcount();
        if (send(data_fd, buf, static_cast<size_t>(n), 0) <= 0) break;
    }

    in.close();
    ::close(data_fd);
    sendResponse(conn, 226, "Transfer complete");
    return true;
}

bool FtpHandler::handleList(IConnection& conn) {
    if (!loggedIn(conn)) return true;
    if (data_listen_fd_ < 0) {
        sendResponse(conn, 425, "Use PASV first");
        return true;
    }

    sendResponse(conn, 150, "Opening data connection for LIST");
    int data_fd = accept(data_listen_fd_, nullptr, nullptr);
    closeDataSocket();

    if (data_fd < 0) {
        sendResponse(conn, 425, "Data connection failed");
        return true;
    }

    std::string listing;
    for (const auto& entry : fs::directory_iterator(userRoot())) {
        listing += (entry.is_directory() ? "DIR  " : "FILE ");
        listing += entry.path().filename().string();
        listing += "\r\n";
    }
    send(data_fd, listing.data(), listing.size(), 0);
    ::close(data_fd);

    sendResponse(conn, 226, "Directory send complete");
    return true;
}

bool FtpHandler::handleQuit(IConnection& conn) {
    state_ = SessionState::UPDATE;
    sendResponse(conn, 221, "Goodbye");
    return false;
}
