#ifndef STORAGE_I_USER_REPOSITORY_HPP
#define STORAGE_I_USER_REPOSITORY_HPP

#include "storage/user.hpp"
#include <string>

class IUserRepository {
public:
    virtual ~IUserRepository() = default;
    virtual bool isConnected() const = 0;
    virtual bool findUser(const std::string& username, User& result) = 0;
    virtual bool createUser(const std::string& username,
                            const std::string& passwordHash, User& result) = 0;
    virtual bool deleteUser(std::int64_t id) = 0;
};

#endif
