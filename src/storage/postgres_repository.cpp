#include "storage/postgres_repository.hpp"

#include <cstdlib>
#include <iostream>
#include <string>

namespace {
std::int64_t toInt64(const char* value) {
    return value ? std::stoll(value) : 0;
}
}

PostgresRepository::PostgresRepository() {
    const char* conninfo = "";
    connection_ = PQconnectdb(conninfo ? conninfo : "");
    if (!isConnected()) {
        std::cerr << "PostgreSQL connection failed: "
                  << (connection_ ? PQerrorMessage(connection_) : "no connection") << '\n';
    }
}

PostgresRepository::~PostgresRepository() {
    if (connection_) PQfinish(connection_);
}

bool PostgresRepository::isConnected() const {
    return connection_ && PQstatus(connection_) == CONNECTION_OK;
}

PGresult* PostgresRepository::execute(const char* sql, int count, const char* const* values) {
    if (!isConnected()) return nullptr;
    PGresult* result = PQexecParams(connection_, sql, count, nullptr, values, nullptr, nullptr, 0);
    if (!result || (PQresultStatus(result) != PGRES_TUPLES_OK &&
                    PQresultStatus(result) != PGRES_COMMAND_OK)) {
        std::cerr << "PostgreSQL query failed: " << PQerrorMessage(connection_) << '\n';
        if (result) PQclear(result);
        return nullptr;
    }
    return result;
}

bool PostgresRepository::findUser(const std::string& username, User& user) {
    const char* values[] = {username.c_str()};
    PGresult* result = execute(
        "SELECT id, username, password_hash, storage_path FROM users WHERE username = $1",
        1, values);
    if (!result) return false;
    const bool found = PQntuples(result) == 1;
    if (found) {
        user.id = toInt64(PQgetvalue(result, 0, 0));
        user.username = PQgetvalue(result, 0, 1);
        user.password_hash = PQgetvalue(result, 0, 2);
        user.storage_path = PQgetvalue(result, 0, 3);
    }
    PQclear(result);
    return found;
}

bool PostgresRepository::createUser(const std::string& username,
                                    const std::string& passwordHash, User& user) {
    const std::string storagePath = "storage/" + username;
    const char* values[] = {username.c_str(), passwordHash.c_str(), storagePath.c_str()};
    PGresult* result = execute(
        "INSERT INTO users (username, password_hash, storage_path) VALUES ($1, $2, $3) RETURNING id",
        3, values);
    if (!result) return false;
    const bool created = PQntuples(result) == 1;
    if (created) {
        user.id = toInt64(PQgetvalue(result, 0, 0));
        user.username = username;
        user.password_hash = passwordHash;
        user.storage_path = storagePath;
    }
    PQclear(result);
    return created;
}

bool PostgresRepository::deleteUser(std::int64_t id) {
    const std::string idText = std::to_string(id);
    const char* values[] = {idText.c_str()};
    PGresult* result = execute("DELETE FROM users WHERE id = $1", 1, values);
    if (!result) return false;
    PQclear(result);
    return true;
}

bool PostgresRepository::findFile(std::int64_t ownerId, const std::string& path,
                                  FileMetadata& file) {
    const std::string owner = std::to_string(ownerId);
    const char* values[] = {owner.c_str(), path.c_str()};
    PGresult* result = execute(
        "SELECT id, owner_id, path, size_bytes FROM files WHERE owner_id = $1 AND path = $2",
        2, values);
    if (!result) return false;
    const bool found = PQntuples(result) == 1;
    if (found) {
        file.id = toInt64(PQgetvalue(result, 0, 0));
        file.owner_id = toInt64(PQgetvalue(result, 0, 1));
        file.path = PQgetvalue(result, 0, 2);
        file.size_bytes = toInt64(PQgetvalue(result, 0, 3));
    }
    PQclear(result);
    return found;
}

bool PostgresRepository::upsertFile(std::int64_t ownerId, const std::string& path,
                                    std::int64_t sizeBytes) {
    const std::string owner = std::to_string(ownerId);
    const std::string size = std::to_string(sizeBytes);
    const char* values[] = {owner.c_str(), path.c_str(), size.c_str()};
    PGresult* result = execute(
        "INSERT INTO files (owner_id, path, size_bytes) VALUES ($1, $2, $3) "
        "ON CONFLICT (owner_id, path) DO UPDATE SET size_bytes = EXCLUDED.size_bytes",
        3, values);
    if (!result) return false;
    PQclear(result);
    return true;
}

bool PostgresRepository::deleteFile(std::int64_t ownerId, const std::string& path) {
    const std::string owner = std::to_string(ownerId);
    const char* values[] = {owner.c_str(), path.c_str()};
    PGresult* result = execute("DELETE FROM files WHERE owner_id = $1 AND path = $2", 2, values);
    if (!result) return false;
    PQclear(result);
    return true;
}

bool PostgresRepository::listFiles(std::int64_t ownerId, std::vector<FileMetadata>& files) {
    const std::string owner = std::to_string(ownerId);
    const char* values[] = {owner.c_str()};
    PGresult* result = execute(
        "SELECT id, owner_id, path, size_bytes FROM files WHERE owner_id = $1 ORDER BY path",
        1, values);
    if (!result) return false;
    files.clear();
    for (int row = 0; row < PQntuples(result); ++row) {
        FileMetadata file;
        file.id = toInt64(PQgetvalue(result, row, 0));
        file.owner_id = toInt64(PQgetvalue(result, row, 1));
        file.path = PQgetvalue(result, row, 2);
        file.size_bytes = toInt64(PQgetvalue(result, row, 3));
        files.push_back(file);
    }
    PQclear(result);
    return true;
}
