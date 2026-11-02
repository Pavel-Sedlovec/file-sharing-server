#include "business/user_service.hpp"
#include "storage/password_hash.hpp"

#include <filesystem>
#include <system_error>

bool UserService::databaseAvailable() const {
    return repository_.isConnected();
}

bool UserService::loadUser(const std::string& username, User& user) {
    return repository_.findUser(username, user);
}

bool UserService::authenticate(const std::string& username, const std::string& password) {
    User user;
    return loadUser(username, user) && verifyPassword(password, user.password_hash);
}

bool UserService::userExists(const std::string& username) {
    User user;
    return loadUser(username, user);
}

bool UserService::validUsername(const std::string& username) const {
    if (username.empty() || username.size() > 64) return false;
    for (unsigned char c : username) {
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.'))
            return false;
    }
    return username != "." && username != "..";
}

std::string UserService::registerUser(const std::string& username,
                                      const std::string& password) {

    if(!repository_.isConnected()){
        return "no";
    }                                    
    if (!repository_.isConnected() || !validUsername(username))
        return {};

    const std::string hash = hashPassword(password);
    if (hash.empty()) return {};

    User user;
    if (!repository_.createUser(username, hash, user)) return {};
    std::error_code error;
    std::filesystem::create_directories(user.storage_path, error);
    if (error) {
        repository_.deleteUser(user.id);
        return {};
    }
    return user.username;
}

bool UserService::prepareUserStorage(const std::string& username) {
    User user;
    if (!loadUser(username, user)) return false;
    std::error_code error;
    std::filesystem::create_directories(user.storage_path, error);
    return !error && std::filesystem::is_directory(user.storage_path);
}
