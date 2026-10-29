#ifndef STORAGE_POSTGRES_REPOSITORY_HPP
#define STORAGE_POSTGRES_REPOSITORY_HPP

#include "storage/i_file_repository.hpp"
#include "storage/i_user_repository.hpp"
#include <libpq-fe.h>

class PostgresRepository final : public IUserRepository, public IFileRepository {
public:
    PostgresRepository();
    ~PostgresRepository() override;
    PostgresRepository(const PostgresRepository&) = delete;
    PostgresRepository& operator=(const PostgresRepository&) = delete;

    bool isConnected() const override;
    bool findUser(const std::string& username, User& result) override;
    bool createUser(const std::string& username, const std::string& passwordHash,
                    User& result) override;
    bool deleteUser(std::int64_t id) override;
    bool findFile(std::int64_t ownerId, const std::string& path,
                  FileMetadata& result) override;
    bool upsertFile(std::int64_t ownerId, const std::string& path,
                    std::int64_t sizeBytes) override;
    bool deleteFile(std::int64_t ownerId, const std::string& path) override;
    bool listFiles(std::int64_t ownerId, std::vector<FileMetadata>& result) override;

private:
    PGconn* connection_ = nullptr;
    PGresult* execute(const char* sql, int count = 0, const char* const* values = nullptr);
};

#endif
