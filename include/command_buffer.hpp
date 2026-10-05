#ifndef COMMAND_BUFFER_HPP
#define COMMAND_BUFFER_HPP

#include <string>
#include <optional>

struct Command {
    std::string name;
    std::string arg;
};

class CommandBuffer {
private:
    std::string _buffer;
    size_t _max_buffer_size;

public:
    CommandBuffer(size_t max_size = 4096);

    bool append(const char* data, size_t len);
    std::optional<Command> pop_command();
};

#endif