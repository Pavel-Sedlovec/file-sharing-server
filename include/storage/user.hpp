#ifndef STORAGE_USER_HPP
#define STORAGE_USER_HPP

#include <cstdint>
#include <string>

struct User {
    std::int64_t id = 0;
    std::string username;
    std::string password_hash;
    std::string storage_path;
};

#endif
