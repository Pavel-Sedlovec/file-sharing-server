#include "protocol/ftp_handler.hpp"
#include "business/ftp_service.hpp"

bool FtpHandler::handleDele(const std::string& filename, IConnection& conn) {
    if (!loggedIn(conn)) return true;
    if (filename.empty()) {
        sendResponse(conn, 501, "DELE requires filename");
        return true;
    }
    const bool success = service_->deleteFile(current_user_, filename);
    sendResponse(conn, success ? 250 : 550,
                 success ? "File deleted" : "File not found or delete failed");
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
