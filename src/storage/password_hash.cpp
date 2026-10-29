#include "storage/password_hash.hpp"

#include <crypt.h>
#include <fstream>
#include <array>

namespace {
std::string makeSalt() {
    static const char alphabet[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789./";
    std::array<unsigned char, 16> bytes{};
    std::ifstream random("/dev/urandom", std::ios::binary);
    if (!random.read(reinterpret_cast<char*>(bytes.data()), bytes.size())) return {};
    std::string salt;
    salt.reserve(bytes.size());
    for (unsigned char byte : bytes) salt.push_back(alphabet[byte % (sizeof(alphabet) - 1)]);
    return salt;
}
}

std::string hashPassword(const std::string& password) {
    const std::string salt = makeSalt();
    if (salt.empty()) return {};
    const std::string setting = "$6$rounds=100000$" + salt + "$";
    crypt_data data{};
    char* encoded = crypt_r(password.c_str(), setting.c_str(), &data);
    return encoded ? std::string(encoded) : std::string();
}

bool verifyPassword(const std::string& password, const std::string& hash) {
    if (hash.empty()) return false;
    crypt_data data{};
    char* encoded = crypt_r(password.c_str(), hash.c_str(), &data);
    return encoded && hash == encoded;
}
