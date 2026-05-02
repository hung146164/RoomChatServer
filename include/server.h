#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>

class Server
{
private:
    int server_fd;
    int port;
    fd_set master_set;
    int max_fd;

public:
    Server(int port);
    ~Server();
    void start();
};