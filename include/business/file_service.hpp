#ifndef FILE_SERVICE_HPP
#define FILE_SERVICE_HPP

#include "protocol/connection_interfaces.hpp"
#include "storage/postgres_repository.hpp"
#include <string>

class FileService {
public:
    explicit FileService(PostgresRepository& repository) : repository_(repository) {}

    bool storeFile(const std::string& username, const std::string& filename,
                   IDataConnection& dataConnection);
    bool retrieveFile(const std::string& username, const std::string& filename,
                      IDataConnection& dataConnection);
    bool deleteFile(const std::string& username, const std::string& filename);
    bool sendDirectoryListing(const std::string& username,
                              IDataConnection& dataConnection);

private:
    PostgresRepository& repository_;
    bool loadActiveUser(const std::string& username, User& user);
    std::string safeFilename(const std::string& filename) const;
};

#endif
