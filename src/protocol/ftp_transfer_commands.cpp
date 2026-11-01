#include "protocol/ftp_handler.hpp"
#include "business/ftp_service.hpp"

#include <sstream>

bool FtpHandler::handlePasv(IConnection& conn) {
    if (!loggedIn(conn)) return true;
    const auto endpoint = conn.openPassiveDataChannel();
    if (!endpoint) {
        sendResponse(conn, 425, "Cannot open passive connection");
        return true;
    }

    unsigned int a = 127, b = 0, c = 0, d = 1;
    char dot1 = 0, dot2 = 0, dot3 = 0;
    std::istringstream ip(endpoint->ip);
    if (!(ip >> a >> dot1 >> b >> dot2 >> c >> dot3 >> d)) {
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

bool FtpHandler::handleStor(const std::string& filename, IConnection& conn) {
    if (!loggedIn(conn)) return true;
    if (filename.empty()) {
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
    const bool success = service_->storeFile(current_user_, filename, *data);
    data->close();
    delete data;
    sendResponse(conn, success ? 226 : 550,
                 success ? "Transfer complete" : "Cannot store file or transfer failed");
    return true;
}

bool FtpHandler::handleRetr(const std::string& filename, IConnection& conn) {
    if (!loggedIn(conn)) return true;
    if (filename.empty()) {
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
    const bool success = service_->retrieveFile(current_user_, filename, *data);
    data->close();
    delete data;
    sendResponse(conn, success ? 226 : 550,
                 success ? "Transfer complete" : "File not found or transfer failed");
    return true;
}
