#include "server.hpp"
#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <future>
#include <signal.h>
#include <atomic>
#include <thread>

std::atomic<bool> stop_server{false};

void server_stop(int sig){
    if(sig == SIGINT){
        stop_server = true;
    }
}

bool check_exit(const char* buffer, size_t len) {
    if (len < 4) return false;
    
    return *(buffer + 0) == 'e' && 
           *(buffer + 1) == 'x' && 
           *(buffer + 2) == 'i' && 
           *(buffer + 3) == 't';
}

bool send_all(int sock, const char* data, size_t len) {
    size_t sent_total = 0;
    while (sent_total < len) {
        ssize_t sent = send(sock, data + sent_total, len - sent_total, 0);
        if (sent <= 0) {
            return false;
        }
        sent_total += static_cast<size_t>(sent);
    }
    return true;
}

int start_client(int client_sock){
    char buffer[BUFFER_SIZE];

    struct timeval tv{};
    tv.tv_sec = 300;
    tv.tv_usec = 0;
    setsockopt(client_sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    while(true){
        ssize_t bytes = recv(client_sock, &buffer[0], BUFFER_SIZE, 0);       
        if(bytes <= 0){
            break;
        }
        if(check_exit(buffer, static_cast<size_t>(bytes))){
            break;
        }
        if(!send_all(client_sock,buffer, static_cast<size_t>(bytes))){
            break;
        }
    }
    close(client_sock);
    return 0;
}

void setup_signals() {
    struct sigaction sa{};
    sa.sa_handler = server_stop;
    sigaction(SIGINT, &sa, nullptr);
}

int create_and_bind_socket(sockaddr_in& server_addr) {
    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if(server_sock < 0){
        return -1;
    }
    
    int opt = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    int ret = bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr));  
    if(ret < 0){
        close(server_sock);
        return -1;
    }
    return server_sock;
}

void run_server(int server_sock) {
    sockaddr_in client_addr;
    socklen_t client_size = sizeof(client_addr);

    
    while(!stop_server){
        int client_sock = accept(server_sock, (sockaddr*)&client_addr, &client_size);
        if (client_sock < 0) {
            if (errno == EINTR) {
                break;
            }
            continue;
        }
        std::thread(start_client, client_sock).detach();

    }   
}
