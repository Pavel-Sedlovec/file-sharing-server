#include "business/ftp_service.hpp"

FtpService::FtpService()
    : repository_(), users_(repository_), files_(repository_) {}

bool FtpService::databaseAvailable() const { return users_.databaseAvailable(); }
bool FtpService::authenticate(const std::string& user, const std::string& password) {
    return users_.authenticate(user, password);
}
std::string FtpService::registerUser(const std::string& username, const std::string& password) {
    return users_.registerUser(username, password);
}
bool FtpService::userExists(const std::string& user) { return users_.userExists(user); }
bool FtpService::prepareUserStorage(const std::string& user) { return users_.prepareUserStorage(user); }
bool FtpService::storeFile(const std::string& user, const std::string& filename, IDataConnection& data) {
    return files_.storeFile(user, filename, data);
}
bool FtpService::retrieveFile(const std::string& user, const std::string& filename, IDataConnection& data) {
    return files_.retrieveFile(user, filename, data);
}
bool FtpService::deleteFile(const std::string& user, const std::string& filename) {
    return files_.deleteFile(user, filename);
}
bool FtpService::sendDirectoryListing(const std::string& user, IDataConnection& data) {
    return files_.sendDirectoryListing(user, data);
}
