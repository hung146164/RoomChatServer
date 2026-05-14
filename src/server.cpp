#include "../header/networkcommon.h"

// network config
const int port = 8080;

// room config
const int listenSize = 5;
const int MAX_EVENTS = 10;
const int MAX_FDS = 100000;

const int MAX_PLAYERS = 1000;
struct ClientDetail
{
    int32_t serverfd;
    int32_t currentClientfd;
    int32_t currentConnections;
    std::vector<bool> isConnected;
    ClientDetail()
    {
        serverfd = -1;
        currentClientfd = -1;
        currentConnections = 0;
        isConnected.resize(MAX_FDS, 0);
    }
};
bool ConfigServerfd(ClientDetail *clientDetail)
{
    clientDetail->serverfd = socket(AF_INET, SOCK_STREAM, 0);
    if (clientDetail->serverfd <= 0)
    {
        std::cerr << "Create socket failed" << '\n';
        return false;
    }

    std::cerr << "Socket created" << '\n';

    int opt = 1;
    if (setsockopt(clientDetail->serverfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        std::cerr << "setsockopt(SO_REUSEADDR) failed" << '\n';
    }
    return true;
}
bool ConfigServerAddrAndBind(ClientDetail *clientDetail)
{
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    if (bind(clientDetail->serverfd, (sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        std::cerr << "Bind error!" << '\n';
        return false;
    }
    else
    {
        std::cerr << "Bind done!" << '\n';
    }
    return true;
}
bool StartListen(ClientDetail *clientDetail)
{
    if (listen(clientDetail->serverfd, listenSize) < 0)
    {
        std::cerr << "Listen failed!";
        return false;
    }
    else
    {
        std::cerr << "Listen at port " << port << '\n';
    }
    return true;
}

int main()
{
    ClientDetail *clientdetail = new ClientDetail();
    if (!ConfigServerfd(clientdetail))
    {
        delete clientdetail;
        return 1;
    }
    if (!ConfigServerAddrAndBind(clientdetail))
    {
        close(clientdetail->serverfd);
        delete clientdetail;
        return 1;
    }
    if (!StartListen(clientdetail))
    {
        close(clientdetail->serverfd);
        delete clientdetail;
        return 1;
    }

    int epollfd = epoll_create1(0);
    if (epollfd == -1)
    {
        std::cerr << "Create epoll failed" << '\n';
        return 1;
    }

    struct epoll_event ev;
    struct epoll_event events[MAX_EVENTS];

    ev.events = EPOLLIN;
    ev.data.fd = clientdetail->serverfd;

    // đăng ký add vào bảng
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, clientdetail->serverfd, &ev) == -1)
    {
        std::cerr << "Failed to add server to epoll'\n";
    }

    while (true)
    {
        int nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        if (nfds == -1)
        {
            std::cerr << "Epoll wait error!\n";
            continue;
        }
        for (int i = 0; i < nfds; i++)
        {
            int current_fd = events[i].data.fd;
            if (current_fd == clientdetail->serverfd)
            {
                clientdetail->currentClientfd = accept(clientdetail->serverfd, NULL, NULL);
                if (clientdetail->currentClientfd >= 0)
                {
                    int new_fd = clientdetail->currentClientfd;

                    if (new_fd >= MAX_FDS)
                    {
                        std::cerr << "FD limit reached, forcefully closing.\n";
                        close(new_fd);
                        continue;
                    }
                    if (clientdetail->currentConnections >= MAX_PLAYERS)
                    {
                        const char *rejectMsg = "Server is full! Please try again later.\n";
                        write(new_fd, rejectMsg, strlen(rejectMsg));
                        close(new_fd);
                        std::cout << "Rejected player at fd " << new_fd << " due to max capacity.\n";
                        continue;
                    }
                    std::cout << "New client socket id: " << clientdetail->currentClientfd << '\n';

                    clientdetail->isConnected[new_fd] = true;
                    clientdetail->currentConnections++;
                    std::cout << "Current players online: " << clientdetail->currentConnections << "/" << MAX_PLAYERS << "\n";

                    ev.events = EPOLLIN;
                    ev.data.fd = clientdetail->currentClientfd;
                    epoll_ctl(epollfd, EPOLL_CTL_ADD, clientdetail->currentClientfd, &ev);
                }
                else
                {
                    std::cerr << "Accepted failed" << '\n';
                }
            }
            else
            {
                char message[1024];
                int valread = read(current_fd, message, 1024);

                if (valread == 0 || valread < 0)
                {
                    if (valread == 0)
                        std::cout << current_fd << " Disconnected!" << '\n';
                    else
                    {
                        std::cerr << "Read error at " << current_fd << " force quit" << '\n';
                    }
                    epoll_ctl(epollfd, EPOLL_CTL_DEL, current_fd, NULL);
                    close(current_fd);

                    if (clientdetail->isConnected[current_fd])
                    {
                        clientdetail->isConnected[current_fd] = false;
                        clientdetail->currentConnections--;
                        std::cout << "Current players online: " << clientdetail->currentConnections << "/" << MAX_PLAYERS << "\n";
                    }
                }
                else
                {
                    message[valread] = '\0';
                    std::cout << "message from " << current_fd << ": " << message << '\n';

                    int bytesSent = send(current_fd, message, valread, MSG_NOSIGNAL);

                    if (bytesSent < 0)
                    {
                        std::cerr << "Failed to send echo back to " << current_fd << '\n';
                    }
                }
            }
        }
    }

    close(epollfd);
    close(clientdetail->serverfd);

    for (int i = 0; i < MAX_FDS; i++)
    {
        if (clientdetail->isConnected[i])
        {
            close(i);
        }
    }

    delete clientdetail;
    return 0;
}