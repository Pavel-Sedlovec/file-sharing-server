#include "protocol/command_buffer.hpp"
#include "storage/password_hash.hpp"
#include "storage/user.hpp"
#include "storage/file_metadata.hpp"

#include <cstdlib>
#include <iostream>
#include <string>

namespace {
int failures = 0;
int checks = 0;

void expect(bool condition, const char* name) {
    ++checks;
    if (!condition) {
        ++failures;
        std::cerr << "[FAIL] " << name << '\n';
    } else {
        std::cout << "[PASS] " << name << '\n';
    }
}

void testCommandBuffer() {
    {
        CommandBuffer buffer;
        expect(buffer.append("NOOP\r\n", 6), "command buffer accepts a complete command");
        auto command = buffer.pop_command();
        expect(command.has_value(), "command buffer extracts a complete command");
        expect(command && command->name == "NOOP", "command name parsed without arguments");
        expect(command && command->arg.empty(), "argument is empty when omitted");
        expect(!buffer.pop_command().has_value(), "empty buffer has no command");
    }
    {
        CommandBuffer buffer;
        expect(buffer.append("USER al", 7), "command buffer accepts a partial command");
        expect(!buffer.pop_command().has_value(), "partial command is retained until newline");
        expect(buffer.append("ice\r\nPASS secret\n", 17), "command buffer appends remaining bytes");
        auto first = buffer.pop_command();
        auto second = buffer.pop_command();
        expect(first && first->name == "USER" && first->arg == "alice",
               "command and argument parsed across appends");
        expect(second && second->name == "PASS" && second->arg == "secret",
               "multiple commands parsed from one append");
    }
    {
        CommandBuffer buffer(5);
        expect(buffer.append("12345", 5), "buffer accepts data at its size limit");
        expect(!buffer.append("x", 1), "buffer rejects data above its size limit");
        expect(!buffer.pop_command().has_value(), "buffer does not emit a command without newline");
    }
    {
        CommandBuffer buffer;
        expect(buffer.append("  arg with spaces\n", 18), "buffer accepts arguments containing spaces");
        auto command = buffer.pop_command();
        expect(command && command->name.empty() && command->arg == " arg with spaces",
               "parser splits command at first space only");
    }
}

void testPasswordHash() {
    const std::string password = "correct horse battery staple";
    const std::string hash = hashPassword(password);
    expect(!hash.empty(), "password hashing returns a hash");
    expect(hash != password, "password hash does not expose the plain-text password");
    expect(verifyPassword(password, hash), "correct password verifies");
    expect(!verifyPassword("wrong password", hash), "incorrect password is rejected");
    expect(!verifyPassword(password, ""), "empty hash is rejected");
    const std::string secondHash = hashPassword(password);
    expect(!secondHash.empty() && secondHash != hash,
           "hashing the same password uses a fresh salt");
    expect(verifyPassword(password, secondHash), "password verifies against second salted hash");
}

void testDataModels() {
    User user;
    expect(user.id == 0 && user.username.empty() && user.password_hash.empty() &&
               user.storage_path.empty(),
           "User fields have safe default values");
    user.id = 42;
    user.username = "alice";
    expect(user.id == 42 && user.username == "alice", "User fields can store user data");

    FileMetadata metadata;
    expect(metadata.id == 0 && metadata.owner_id == 0 && metadata.path.empty() &&
               metadata.size_bytes == 0,
           "FileMetadata fields have safe default values");
    metadata.owner_id = 42;
    metadata.path = "report.txt";
    metadata.size_bytes = 128;
    expect(metadata.owner_id == 42 && metadata.path == "report.txt" &&
               metadata.size_bytes == 128,
           "FileMetadata fields can store file metadata");
}
}

int main() {
    testCommandBuffer();
    testPasswordHash();
    testDataModels();

    std::cout << "\n" << (checks - failures) << "/" << checks << " checks passed\n";
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
