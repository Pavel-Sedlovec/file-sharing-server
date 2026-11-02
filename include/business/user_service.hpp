#ifndef USER_SERVICE_HPP
#define USER_SERVICE_HPP

#include "storage/postgres_repository.hpp"
#include <string>

class UserService {
public:
    explicit UserService(PostgresRepository& repository) : repository_(repository) {}

    bool databaseAvailable() const;
    bool authenticate(const std::string& username, const std::string& password);
    std::string registerUser(const std::string& username, const std::string& password);
    bool userExists(const std::string& username);
    bool prepareUserStorage(const std::string& username);

private:
    PostgresRepository& repository_;
    bool loadUser(const std::string& username, User& user);
    bool validUsername(const std::string& username) const;
};

#endif
