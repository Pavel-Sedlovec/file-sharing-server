#ifndef STORAGE_I_FILE_REPOSITORY_HPP
#define STORAGE_I_FILE_REPOSITORY_HPP

#include "storage/file_metadata.hpp"
#include <cstdint>
#include <string>
#include <vector>

class IFileRepository {
public:
    virtual ~IFileRepository() = default;
    virtual bool findFile(std::int64_t ownerId, const std::string& path,
                          FileMetadata& result) = 0;
    virtual bool upsertFile(std::int64_t ownerId, const std::string& path,
                            std::int64_t sizeBytes) = 0;
    virtual bool deleteFile(std::int64_t ownerId, const std::string& path) = 0;
    virtual bool listFiles(std::int64_t ownerId, std::vector<FileMetadata>& result) = 0;
};

#endif
