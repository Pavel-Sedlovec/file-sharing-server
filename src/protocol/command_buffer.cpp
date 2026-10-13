#include "protocol/command_buffer.hpp"

CommandBuffer::CommandBuffer(size_t max_size) : _max_buffer_size(max_size) {}

bool CommandBuffer::append(const char* data, size_t len) {
    if (_buffer.size() + len > _max_buffer_size) {
        return false;
    }
    _buffer.append(data, len);
    return true;
}

std::optional<Command> CommandBuffer::pop_command() {
    size_t pos = _buffer.find('\n');
if (pos == std::string::npos) {
    return std::nullopt;
}

std::string line = _buffer.substr(0, pos);
_buffer.erase(0, pos + 1);

if (!line.empty() && line.back() == '\r') {
    line.pop_back();
}

    Command cmd;
    size_t space_pos = line.find(' ');
    if (space_pos != std::string::npos) {
        cmd.name = line.substr(0, space_pos);
        cmd.arg = line.substr(space_pos + 1);
    } else {
        cmd.name = line;
    }

    return cmd;
}