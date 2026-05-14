/* Author: HungForree - AC stable version */
#include "../header/networkcommon.h"

int NUM_BOTS = 100;
int PORT = 8080;
std::string SERVER_IP = "137.184.77.180";

// ---------------- CONFIG ----------------
void LoadConfig()
{
    char *serverSecret = std::getenv("SERVER_IP");
    char *portStr = std::getenv("SERVER_PORT");
    if (serverSecret != nullptr)
    {
        SERVER_IP = std::getenv("SERVER_IP");
    }
    if (portStr != nullptr)
    {
        PORT = std::stoi(portStr);
    }
}

// ---------------- STRUCT ----------------
struct NetworkMetrics
{
    std::vector<long long> latencies;
    int total_sent = 0;
    int total_received = 0;
};

struct BotContext
{
    int id;
    int msg_count = 0;
    std::chrono::high_resolution_clock::time_point last_send;
};

// ---------------- UTIL ----------------
void setNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

// ---------------- CLOSE BOT SAFE ----------------
struct Runtime
{
    int epoll_fd;
    std::map<int, BotContext> *bots;
    NetworkMetrics *stats;
    int *completed;
};

void close_bot(Runtime &rt, int fd)
{
    epoll_ctl(rt.epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
    close(fd);
    (*rt.completed)++;
}

// ---------------- CONNECT ----------------
bool start_connect(int sock, sockaddr_in &addr)
{
    int ret = connect(sock, (sockaddr *)&addr, sizeof(addr));
    if (ret == 0)
        return true;
    if (errno == EINPROGRESS)
        return true;
    return false;
}

// ---------------- MAIN ----------------
int main()
{
    LoadConfig();

    int num_bots;
    std::cout << "Enter number of bots: ";
    std::cin >> num_bots;

    int epoll_fd = epoll_create1(0);

    std::map<int, BotContext> bots;
    NetworkMetrics stats;

    std::vector<epoll_event> events(num_bots);

    int completed = 0;

    Runtime rt{epoll_fd, &bots, &stats, &completed};

    // ---------------- CREATE BOTS ----------------
    for (int i = 0; i < num_bots; i++)
    {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        setNonBlocking(sock);

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(PORT);
        inet_pton(AF_INET, SERVER_IP.c_str(), &addr.sin_addr);

        start_connect(sock, addr);

        epoll_event ev{};
        ev.data.fd = sock;
        ev.events = EPOLLOUT | EPOLLET;

        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock, &ev);

        bots[sock] = {i, 0, std::chrono::high_resolution_clock::now()};
    }

    auto start_time = std::chrono::high_resolution_clock::now();

    // ---------------- LOOP ----------------
    while (completed < num_bots)
    {
        int nfds = epoll_wait(epoll_fd, events.data(), num_bots, 10);

        for (int i = 0; i < nfds; i++)
        {
            int fd = events[i].data.fd;

            // ERROR HANDLING FIRST
            if (events[i].events & (EPOLLERR | EPOLLHUP))
            {
                std::cout << "[BOT " << bots[fd].id << "] EPOLLERR/HUP\n";
                close_bot(rt, fd);
                continue;
            }

            // ---------------- CONNECTED ----------------
            if (events[i].events & EPOLLOUT)
            {
                int err = 0;
                socklen_t len = sizeof(err);
                getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len);

                if (err != 0)
                {
                    std::cout << "[BOT " << bots[fd].id
                              << "] connect failed: "
                              << strerror(err) << "\n";
                    close_bot(rt, fd);
                    continue;
                }

                std::string msg =
                    "Ping bot " + std::to_string(bots[fd].id);

                bots[fd].last_send =
                    std::chrono::high_resolution_clock::now();

                int sent = 0;
                while (sent < (int)msg.size())
                {
                    int n = send(fd, msg.data() + sent,
                                 msg.size() - sent, 0);

                    if (n > 0)
                    {
                        sent += n;
                        stats.total_sent++;
                    }
                    else if (n == -1 && errno == EAGAIN)
                        break;
                    else
                    {
                        close_bot(rt, fd);
                        goto next_event;
                    }
                }

                epoll_event ev{};
                ev.data.fd = fd;
                ev.events = EPOLLIN | EPOLLET;
                epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &ev);
            }

            // ---------------- READ ----------------
            else if (events[i].events & EPOLLIN)
            {
                char buf[1024];

                while (true)
                {
                    int n = recv(fd, buf, sizeof(buf), 0);

                    if (n > 0)
                    {
                        auto end =
                            std::chrono::high_resolution_clock::now();

                        long long lat =
                            std::chrono::duration_cast<
                                std::chrono::microseconds>(
                                end - bots[fd].last_send)
                                .count();

                        stats.latencies.push_back(lat);
                        stats.total_received++;
                        bots[fd].msg_count++;

                        // std::cout << "[BOT " << bots[fd].id
                        //           << "] recv latency="
                        //           << lat << "us count="
                        //           << bots[fd].msg_count << "\n";

                        if (bots[fd].msg_count < 5)
                        {
                            epoll_event ev{};
                            ev.data.fd = fd;
                            ev.events = EPOLLOUT | EPOLLET;
                            epoll_ctl(epoll_fd,
                                      EPOLL_CTL_MOD, fd, &ev);
                        }
                        else
                        {
                            std::cout << "[BOT " << bots[fd].id
                                      << "] DONE\n";
                            close_bot(rt, fd);
                        }
                    }
                    else if (n == 0)
                    {
                        close_bot(rt, fd);
                        break;
                    }
                    else
                    {
                        if (errno == EAGAIN)
                            break;
                        close_bot(rt, fd);
                        break;
                    }
                }
            }

        next_event:
            continue;
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now();

    // ---------------- REPORT ----------------
    std::cout << "\n=========== REPORT ===========\n";
    std::cout << "Target: " << SERVER_IP << ":" << PORT << "\n";
    std::cout << "Bots: " << num_bots << "\n";
    std::cout << "Sent: " << stats.total_sent << "\n";
    std::cout << "Recv: " << stats.total_received << "\n";

    if (!stats.latencies.empty())
    {
        auto [mn, mx] =
            std::minmax_element(stats.latencies.begin(),
                                stats.latencies.end());

        long long sum = std::accumulate(
            stats.latencies.begin(),
            stats.latencies.end(), 0LL);

        double avg =
            (double)sum / stats.latencies.size();

        std::cout << "Min: " << *mn << " us\n";
        std::cout << "Max: " << *mx << " us\n";
        std::cout << "Avg: " << avg << " us\n";
    }

    double sec =
        std::chrono::duration_cast<std::chrono::seconds>(
            end_time - start_time)
            .count();

    std::cout << "Throughput: "
              << stats.total_received / std::max(1.0, sec)
              << " pkt/s\n";

    std::cout << "==============================\n";

    return 0;
}