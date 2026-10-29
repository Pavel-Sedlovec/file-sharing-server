#ifndef STORAGE_FILE_METADATA_HPP
#define STORAGE_FILE_METADATA_HPP

#include <cstdint>
#include <string>

struct FileMetadata {
    std::int64_t id = 0;
    std::int64_t owner_id = 0;
    std::string path;
    std::int64_t size_bytes = 0;
};

#endif
