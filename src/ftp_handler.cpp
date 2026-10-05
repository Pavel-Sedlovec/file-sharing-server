#include "ftp_handler.hpp"

void FtpHandler::onConnect(IConnection& conn) { 
    char* ch = "fef"; 
    conn.send(ch, static_cast<size_t>(3));
}

bool FtpHandler::onData(const char* data, size_t len, IConnection& conn) {
    return true; 
}

void FtpHandler::onDisconnect() {
}

FtpHandler::~ftp_protocol() {
}