#ifndef STORAGE_PASSWORD_HASH_HPP
#define STORAGE_PASSWORD_HASH_HPP

#include <string>

std::string hashPassword(const std::string& password);
bool verifyPassword(const std::string& password, const std::string& hash);

#endif
