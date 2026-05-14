/* Author: HungForree */
#include "../header/networkcommon.h"

const int PORT = 8080;
const int MAX_EVENTS = 4096;
const int MAX_FDS = 100000; // Hỗ trợ tối đa 100k kết nối

// ---------------- CẤU TRÚC BỘ NHỚ (O(1) Truy xuất) ----------------
struct Connection
{
    int fd = -1;
    std::string write_buffer;

    // Tái sử dụng vùng nhớ, chống Memory Fragmentation
    void reset(int new_fd)
    {
        fd = new_fd;
        write_buffer.clear();
        write_buffer.reserve(4096); // Cấp phát sẵn 4KB
    }
};

// ---------------- HÀM TIỆN ÍCH TỐI ƯU ----------------
void setNonBlocking(int fd)
{
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
}

void setOptimizedSockets(int fd)
{
    int opt = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt)); // Vô hiệu hóa Nagle
}

void closeConnection(int epollfd, std::vector<Connection> &pool, int fd)
{
    if (fd < 0 || fd >= MAX_FDS)
        return;
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, nullptr);
    close(fd);
    pool[fd].fd = -1; // Trả lại object cho Pool
}

// ---------------- LUỒNG XỬ LÝ ĐỘC LẬP (WORKER THREAD) ----------------
void WorkerLoop(int worker_id)
{
    // 1. Mỗi Thread tự tạo một Listener Socket riêng
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // VŨ KHÍ BÍ MẬT: SO_REUSEPORT (Yêu cầu Linux Kernel 3.9+)
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (sockaddr *)&addr, sizeof(addr)) < 0)
    {
        std::cerr << "Thread " << worker_id << " bind failed!\n";
        return;
    }

    listen(server_fd, SOMAXCONN);
    setNonBlocking(server_fd);

    int epollfd = epoll_create1(0);
    epoll_event ev{}, events[MAX_EVENTS];

    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = server_fd;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, server_fd, &ev);

    // Memory Pool: Mảng tĩnh thay cho việc cấp phát động (new/delete)
    std::vector<Connection> conn_pool(MAX_FDS);

    std::cout << "[WORKER " << worker_id << "] Ready and listening.\n";

    // 2. Vòng lặp Epoll tốc độ siêu cao
    while (true)
    {
        int nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);

        for (int i = 0; i < nfds; i++)
        {
            int fd = events[i].data.fd;

            // ================= KẾT NỐI MỚI (KERNEL ĐÃ CHIA TẢI SẴN) =================
            if (fd == server_fd)
            {
                while (true)
                {
                    int client = accept(server_fd, nullptr, nullptr);
                    if (client < 0)
                    {
                        if (errno == EAGAIN || errno == EWOULDBLOCK)
                            break;
                        continue;
                    }

                    if (client >= MAX_FDS)
                    { // Chống tràn Pool
                        close(client);
                        continue;
                    }

                    setNonBlocking(client);
                    setOptimizedSockets(client);

                    // Lấy object từ Pool ra dùng (O(1) Time)
                    conn_pool[client].reset(client);

                    epoll_event cev{};
                    cev.events = EPOLLIN | EPOLLET;
                    cev.data.fd = client;
                    epoll_ctl(epollfd, EPOLL_CTL_ADD, client, &cev);
                }
            }
            // ================= DỮ LIỆU TỪ CLIENT =================
            else
            {
                Connection &conn = conn_pool[fd];
                if (conn.fd == -1)
                    continue; // Tránh lỗi trỏ ngầm

                if (events[i].events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP))
                {
                    closeConnection(epollfd, conn_pool, fd);
                    continue;
                }

                // --- EPOLLIN: XỬ LÝ NHẬN ---
                if (events[i].events & EPOLLIN)
                {
                    char buf[4096];
                    while (true)
                    {
                        int n = recv(fd, buf, sizeof(buf), 0);
                        if (n > 0)
                        {
                            conn.write_buffer.append(buf, n);
                        }
                        else if (n == 0)
                        {
                            closeConnection(epollfd, conn_pool, fd);
                            goto next_event;
                        }
                        else
                        {
                            if (errno == EAGAIN || errno == EWOULDBLOCK)
                                break;
                            closeConnection(epollfd, conn_pool, fd);
                            goto next_event;
                        }
                    }
                }

                // --- EPOLLOUT & BUFFER FLUSH: XỬ LÝ GỬI ---
                if (!conn.write_buffer.empty())
                {
                    int sent = send(fd, conn.write_buffer.data(), conn.write_buffer.size(), MSG_NOSIGNAL);

                    if (sent > 0)
                    {
                        conn.write_buffer.erase(0, sent);
                    }
                    else if (sent < 0 && (errno != EAGAIN && errno != EWOULDBLOCK))
                    {
                        closeConnection(epollfd, conn_pool, fd);
                        continue;
                    }

                    // Quản lý trạng thái Epoll thông minh
                    epoll_event cev{};
                    cev.data.fd = fd;
                    if (!conn.write_buffer.empty())
                    {
                        cev.events = EPOLLIN | EPOLLOUT | EPOLLET; // Vẫn còn dữ liệu, bắt Kernel gọi lại
                        epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &cev);
                    }
                    else
                    {
                        cev.events = EPOLLIN | EPOLLET; // Đã gửi hết, chỉ chờ đọc
                        epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &cev);
                    }
                }

            next_event:
                continue;
            }
        }
    }
}

// ---------------- HÀM MAIN ----------------
int main()
{
    // Tự động phát hiện số lõi CPU của VPS
    unsigned int num_cores = std::thread::hardware_concurrency();
    if (num_cores == 0)
        num_cores = 2; // Dự phòng

    std::cout << "[SYSTEM] Booting Maximum Performance Server...\n";
    std::cout << "[SYSTEM] Detected " << num_cores << " CPU cores. Launching Shared-Nothing Architecture.\n";

    std::vector<std::thread> workers;

    // Bật N luồng độc lập, gánh tải vô cực
    for (unsigned int i = 0; i < num_cores; ++i)
    {
        workers.emplace_back(WorkerLoop, i);
    }

    for (auto &w : workers)
    {
        w.join();
    }

    return 0;
}