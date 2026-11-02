#include "business/file_service.hpp"

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;


bool FileService::loadActiveUser(const std::string& username, User& user) {
    return repository_.findUser(username, user);
}

// For this coursework version, files are stored directly in the user's root directory.
// Only plain filenames are accepted; paths containing directories are rejected.
std::string FileService::safeFilename(const std::string& filename) const {
    if (filename.empty() || filename == "." || filename == ".." ||
        filename.find('/') != std::string::npos || filename.find('\\') != std::string::npos)
        return {};
    return filename;
}

bool FileService::storeFile(const std::string& username, const std::string& filename,
                            IDataConnection& dataConnection) {
    const std::string name = safeFilename(filename);
    User user;
    if (name.empty() || !loadActiveUser(username, user)) return false;

    const fs::path path = fs::path(user.storage_path) / name;
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) return false;

    char buffer[8192];
    std::int64_t size = 0;
    while (true) {
        const IoResult received = dataConnection.receive(buffer, sizeof(buffer));
        if (received.status == IoStatus::END_OF_STREAM) break;
        if (received.status != IoStatus::OK || received.bytes == 0) {
            output.close();
            fs::remove(path);
            return false;
        }
        output.write(buffer, static_cast<std::streamsize>(received.bytes));
        if (!output) {
            output.close();
            fs::remove(path);
            return false;
        }
        size += static_cast<std::int64_t>(received.bytes);
    }
    output.close();
    if (!output) return false;
    return repository_.upsertFile(user.id, name, size);
}

bool FileService::retrieveFile(const std::string& username, const std::string& filename,
                               IDataConnection& dataConnection) {
    const std::string name = safeFilename(filename);
    User user;
    if (name.empty() || !loadActiveUser(username, user)) return false;

    FileMetadata metadata;
    if (!repository_.findFile(user.id, name, metadata)) return false;
    std::ifstream input(fs::path(user.storage_path) / name, std::ios::binary);
    if (!input) return false;

    char buffer[8192];
    while (input.read(buffer, sizeof(buffer)) || input.gcount() > 0) {
        const auto count = static_cast<std::size_t>(input.gcount());
        const IoResult sent = dataConnection.send(buffer, count);
        if (sent.status != IoStatus::OK || sent.bytes != count) return false;
    }
    return !input.bad();
}

bool FileService::deleteFile(const std::string& username, const std::string& filename) {
    const std::string name = safeFilename(filename);
    User user;
    if (name.empty() || !loadActiveUser(username, user)) return false;

    FileMetadata metadata;
    if (!repository_.findFile(user.id, name, metadata)) return false;
    if (!fs::remove(fs::path(user.storage_path) / name)) return false;
    return repository_.deleteFile(user.id, name);
}

bool FileService::sendDirectoryListing(const std::string& username,
                                       IDataConnection& dataConnection) {
    User user;
    if (!loadActiveUser(username, user)) return false;

    std::vector<FileMetadata> files;
    if (!repository_.listFiles(user.id, files)) return false;
    for (const FileMetadata& file : files) {
        const std::string line = "FILE " + file.path + " " +
                                 std::to_string(file.size_bytes) + "\r\n";
        const IoResult sent = dataConnection.send(line.data(), line.size());
        if (sent.status != IoStatus::OK || sent.bytes != line.size()) return false;
    }
    return true;
}
