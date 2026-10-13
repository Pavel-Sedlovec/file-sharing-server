#include "transport/tcp_connection.hpp"
#include <unistd.h>

tcp_connection::tcp_connection(int fd) : _fd(fd){}

bool tcp_connection::send(const char* data, size_t len)
{
    return send_all(_fd,data,len);
}

void tcp_connection::close() 
{
    ::close(_fd);
}

int tcp_connection::get_fd(){
    return _fd;
}

tcp_connection::~tcp_connection()
{

}