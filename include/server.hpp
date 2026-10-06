#ifndef SERVER_HPP
#define SERVER_HPP

#include <netinet/in.h>
#include <cstddef>

constexpr int SERVER_PORT = 8080;
constexpr int BUFFER_SIZE = 1024;
constexpr int BACKLOG_QUEUE = 5;
constexpr int THREAD_POOL_SIZE = 5;

void server_stop(int sig);
bool send_all(int sock, const char* data, size_t len);
bool check_exit(const char* buffer, size_t len);
int start_client(int client_sock);
void setup_signals();
int create_and_bind_socket(sockaddr_in& server_addr);
void run_server(int server_sock);


#endif