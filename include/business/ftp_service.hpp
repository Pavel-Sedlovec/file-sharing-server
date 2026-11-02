#ifndef FTP_SERVICE_HPP
#define FTP_SERVICE_HPP

#include "business/file_service.hpp"
#include "business/user_service.hpp"
#include <string>

// Stable facade used by the protocol layer; delegates each use-case to a focused service.
class FtpService {
public:
    FtpService();
    FtpService(const FtpService&) = delete;
    FtpService& operator=(const FtpService&) = delete;

    bool databaseAvailable() const;
    bool authenticate(const std::string& user, const std::string& password);
    std::string registerUser(const std::string& username, const std::string& password);
    bool userExists(const std::string& user);
    bool prepareUserStorage(const std::string& user);
    bool storeFile(const std::string& user, const std::string& filename, IDataConnection& dataConnection);
    bool retrieveFile(const std::string& user, const std::string& filename, IDataConnection& dataConnection);
    bool deleteFile(const std::string& user, const std::string& filename);
    bool sendDirectoryListing(const std::string& user, IDataConnection& dataConnection);

private:
    PostgresRepository repository_;
    UserService users_;
    FileService files_;
};

#endif
