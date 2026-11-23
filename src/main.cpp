#include "server.hpp"
#include "logger.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

int main(){
    setup_signals();
    app_log::info("Starting file-sharing server");

    sockaddr_in server_addr;
    int server_sock = create_and_bind_socket(server_addr);
    if (server_sock < 0) {
        app_log::error("Server startup failed while binding socket");
        return -1;
    }

    int lis = listen(server_sock, BACKLOG_QUEUE);
    if(lis < 0){
        app_log::error("listen() failed");
        close(server_sock);
        return -1;
    }

    run_server(server_sock);

    close(server_sock);
    app_log::info("Server stopped cleanly");
    return 0;
}
