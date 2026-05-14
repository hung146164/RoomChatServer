#include "../header/networkcommon.h"

#include <iostream>
#include <cstring>
#include <vector>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/epoll.h>

const int PORT = 8080;
const int MAX_EVENTS = 1024;
const int MAX_PLAYERS = 10000;

void setNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int main()
{
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(server_fd, (sockaddr *)&addr, sizeof(addr));
    listen(server_fd, 128);

    setNonBlocking(server_fd);

    int epollfd = epoll_create1(0);

    epoll_event ev{}, events[MAX_EVENTS];
    ev.events = EPOLLIN;
    ev.data.fd = server_fd;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, server_fd, &ev);

    std::cout << "Server running...\n";

    while (true)
    {
        int nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);

        for (int i = 0; i < nfds; i++)
        {
            int fd = events[i].data.fd;

            // ================= ACCEPT =================
            if (fd == server_fd)
            {
                while (true)
                {
                    int client = accept(server_fd, nullptr, nullptr);
                    if (client < 0)
                        break;

                    if (client >= MAX_PLAYERS)
                    {
                        close(client);
                        continue;
                    }

                    setNonBlocking(client);

                    epoll_event cev{};
                    cev.events = EPOLLIN | EPOLLET;
                    cev.data.fd = client;

                    epoll_ctl(epollfd, EPOLL_CTL_ADD, client, &cev);
                }
            }

            // ================= CLIENT =================
            else
            {
                char buf[4096];

                while (true)
                {
                    int n = recv(fd, buf, sizeof(buf), 0);

                    if (n > 0)
                    {
                        // echo back (FAST PATH)
                        int sent = 0;
                        while (sent < n)
                        {
                            int s = send(fd, buf + sent, n - sent, MSG_NOSIGNAL);
                            if (s <= 0)
                                break;
                            sent += s;
                        }
                    }
                    else if (n == 0)
                    {
                        close(fd);
                        break;
                    }
                    else
                    {
                        if (errno == EAGAIN)
                            break;
                        close(fd);
                        break;
                    }
                }
            }
        }
    }

    close(server_fd);
}