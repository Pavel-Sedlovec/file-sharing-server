#include "business/ftp_service.hpp"

#include <filesystem>
#include <fstream>
#include <mutex>
#include <sstream>
#include <unordered_map>

namespace fs = std::filesystem;

namespace {
struct UserRecord {
    std::string password;
    std::string folder;
};

std::mutex users_mutex;
std::unordered_map<std::string, UserRecord> users = {
    {"admin", {"1234", "admin"}}
};
int next_user_id = 1001;
}

bool FtpService::authenticate(const std::string& user, const std::string& password) const {
    std::lock_guard<std::mutex> lock(users_mutex);
    auto it = users.find(user);
    return it != users.end() && it->second.password == password;
}

bool FtpService::userExists(const std::string& user) const {
    std::lock_guard<std::mutex> lock(users_mutex);
    return users.find(user) != users.end();
}

std::string FtpService::registerUser(const std::string& folder, const std::string& password) {
    fs::path folderPath(folder);
    if (folder.empty() || folderPath.filename().string() != folder ||
        folder == "." || folder == ".." || password.empty()) {
        return {};
    }

    std::string login;
    {
        std::lock_guard<std::mutex> lock(users_mutex);
        do {
            login = "user" + std::to_string(next_user_id++);
        } while (users.count(login));
        users[login] = {password, folder};
    }

    std::error_code ec;
    fs::create_directories(fs::path("storage") / folder, ec);
    if (ec) {
        std::lock_guard<std::mutex> lock(users_mutex);
        users.erase(login);
        return {};
    }
    return login;
}

bool FtpService::prepareUserStorage(const std::string& user) const {
    const std::string root = userRoot(user);
    if (root.empty()) return false;
    std::error_code ec;
    fs::create_directories(root, ec);
    return !ec;
}

std::string FtpService::userRoot(const std::string& user) const {
    std::string folder;
    {
        std::lock_guard<std::mutex> lock(users_mutex);
        auto it = users.find(user);
        if (it == users.end()) return {};
        folder = it->second.folder;
    }
    return (fs::path("storage") / folder).string();
}

std::string FtpService::safeFilename(const std::string& filename) const {
    if (filename.empty()) return {};
    fs::path path(filename);
    std::string name = path.filename().string();
    if (name.empty() || name == "." || name == "..") return {};
    return name;
}

bool FtpService::storeFile(const std::string& user, const std::string& filename,
                           IDataConnection& dataConnection) const {
    const std::string name = safeFilename(filename);
    const std::string rootString = userRoot(user);
    if (name.empty() || rootString.empty()) return false;

    std::error_code ec;
    fs::create_directories(rootString, ec);
    if (ec) return false;
    const fs::path root = fs::weakly_canonical(rootString, ec);
    if (ec) return false;
    const fs::path target = fs::weakly_canonical(root / name, ec);
    if (ec || target.parent_path() != root) return false;

    std::ofstream out(target, std::ios::binary);
    if (!out) return false;

    char buffer[8192];
    while (true) {
        IoResult result = dataConnection.receive(buffer, sizeof(buffer));
        if (result.status == IoStatus::END_OF_STREAM) break;
        if (result.status == IoStatus::ERROR || result.bytes == 0) return false;
        out.write(buffer, static_cast<std::streamsize>(result.bytes));
        if (!out) return false;
    }
    out.close();
    return static_cast<bool>(out);
}

bool FtpService::retrieveFile(const std::string& user, const std::string& filename,
                              IDataConnection& dataConnection) const {
    const std::string name = safeFilename(filename);
    const std::string rootString = userRoot(user);
    if (name.empty() || rootString.empty()) return false;

    std::error_code ec;
    const fs::path root = fs::weakly_canonical(rootString, ec);
    if (ec) return false;
    const fs::path target = fs::weakly_canonical(root / name, ec);
    if (ec || target.parent_path() != root || !fs::is_regular_file(target, ec) || ec) return false;

    std::ifstream in(target, std::ios::binary);
    if (!in) return false;
    char buffer[8192];
    while (in.read(buffer, sizeof(buffer)) || in.gcount() > 0) {
        const auto bytes = static_cast<std::size_t>(in.gcount());
        IoResult result = dataConnection.send(buffer, bytes);
        if (result.status != IoStatus::OK || result.bytes != bytes) return false;
    }
    return !in.bad();
}

bool FtpService::sendDirectoryListing(const std::string& user,
                                      IDataConnection& dataConnection) const {
    const std::string root = userRoot(user);
    if (root.empty()) return false;

    std::error_code ec;
    fs::directory_iterator it(root, ec), end;
    if (ec) return false;
    for (; it != end; it.increment(ec)) {
        if (ec) return false;
        std::string line = (it->is_directory(ec) ? "DIR  " : "FILE ") +
                           it->path().filename().string() + "\r\n";
        if (ec) return false;
        IoResult result = dataConnection.send(line.data(), line.size());
        if (result.status != IoStatus::OK || result.bytes != line.size()) return false;
    }
    return true;
}
