#ifndef FTP_SERVICE_HPP
#define FTP_SERVICE_HPP

#include "protocol/connection_interfaces.hpp"
#include <string>

// Business logic: users, authorization and file operations.
// This layer does not create or manipulate operating-system sockets.
class FtpService {
public:
    FtpService() = default;

    bool authenticate(const std::string& user, const std::string& password) const;
    std::string registerUser(const std::string& folder, const std::string& password);
    bool userExists(const std::string& user) const;
    bool prepareUserStorage(const std::string& user) const;

    bool storeFile(const std::string& user, const std::string& filename,
                   IDataConnection& dataConnection) const;
    bool retrieveFile(const std::string& user, const std::string& filename,
                      IDataConnection& dataConnection) const;
    bool sendDirectoryListing(const std::string& user,
                              IDataConnection& dataConnection) const;

private:
    std::string userRoot(const std::string& user) const;
    std::string safeFilename(const std::string& filename) const;
};

#endif
