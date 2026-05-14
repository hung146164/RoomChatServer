/* Author: HungForree */
#include "../header/networkcommon.h"

const int PORT = 8080;
const int MAX_EVENTS = 1024;

// Cấu trúc lưu trạng thái của từng Client
struct Connection
{
    int fd;
    std::string write_buffer;
};

void setNonBlocking(int fd)
{
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
}

void setTCPNoDelay(int fd)
{
    int opt = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));
}

// Hàm giải phóng tài nguyên gọn gàng
void closeConnection(int epollfd, Connection *conn)
{
    epoll_ctl(epollfd, EPOLL_CTL_DEL, conn->fd, nullptr);
    close(conn->fd);
    delete conn;
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
    listen(server_fd, SOMAXCONN); // Tối ưu: Dùng SOMAXCONN thay vì 128

    setNonBlocking(server_fd);

    int epollfd = epoll_create1(0);

    epoll_event ev{}, events[MAX_EVENTS];

    // Server FD không cần buffer, ta nhét luôn fd vào con trỏ bằng ép kiểu
    ev.events = EPOLLIN | EPOLLET;
    ev.data.ptr = reinterpret_cast<void *>(server_fd);
    epoll_ctl(epollfd, EPOLL_CTL_ADD, server_fd, &ev);

    std::cout << "[SYSTEM] Advanced Epoll Server running on port " << PORT << "...\n";

    while (true)
    {
        int nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);

        for (int i = 0; i < nfds; i++)
        {
            // Kiểm tra xem event này là của server_fd hay của Client
            if (events[i].data.ptr == reinterpret_cast<void *>(server_fd))
            {
                // ================= ACCEPT LOOP =================
                while (true)
                {
                    sockaddr_in client_addr{};
                    socklen_t client_len = sizeof(client_addr);
                    int client = accept(server_fd, (sockaddr *)&client_addr, &client_len);

                    if (client < 0)
                    {
                        if (errno == EAGAIN || errno == EWOULDBLOCK)
                            break;
                        continue;
                    }

                    setNonBlocking(client);
                    setTCPNoDelay(client);

                    // Khởi tạo trạng thái cho Client mới
                    Connection *conn = new Connection{client, ""};

                    epoll_event cev{};
                    cev.events = EPOLLIN | EPOLLET; // Chỉ chờ đọc lúc đầu
                    cev.data.ptr = conn;
                    epoll_ctl(epollfd, EPOLL_CTL_ADD, client, &cev);
                }
            }
            else
            {
                // ================= CLIENT EVENT =================
                Connection *conn = static_cast<Connection *>(events[i].data.ptr);

                if (events[i].events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP))
                {
                    closeConnection(epollfd, conn);
                    continue;
                }

                // 1. NẾU CÓ DỮ LIỆU ĐẾN (EPOLLIN)
                if (events[i].events & EPOLLIN)
                {
                    char buf[4096];
                    while (true)
                    {
                        int n = recv(conn->fd, buf, sizeof(buf), 0);
                        if (n > 0)
                        {
                            // Nhận được bao nhiêu, nhét hết vào buffer gửi đi
                            conn->write_buffer.append(buf, n);
                        }
                        else if (n == 0)
                        {
                            closeConnection(epollfd, conn);
                            goto next_event; // Thoát hẳn vòng lặp Client này
                        }
                        else
                        {
                            if (errno == EAGAIN || errno == EWOULDBLOCK)
                                break;
                            closeConnection(epollfd, conn);
                            goto next_event;
                        }
                    }
                }

                // 2. XỬ LÝ GỬI DỮ LIỆU TỪ BUFFER (EPOLLOUT hoặc sau khi nhận)
                if (!conn->write_buffer.empty())
                {
                    int sent = send(conn->fd, conn->write_buffer.data(), conn->write_buffer.size(), MSG_NOSIGNAL);

                    if (sent > 0)
                    {
                        // Xóa phần đã gửi thành công khỏi buffer
                        conn->write_buffer.erase(0, sent);
                    }
                    else if (sent < 0 && (errno != EAGAIN && errno != EWOULDBLOCK))
                    {
                        closeConnection(epollfd, conn);
                        continue;
                    }

                    // Điều chỉnh cờ Epoll dựa trên trạng thái buffer
                    epoll_event cev{};
                    cev.data.ptr = conn;
                    if (!conn->write_buffer.empty())
                    {
                        // Gửi chưa hết (EAGAIN), đăng ký thêm EPOLLOUT để OS gọi lại
                        cev.events = EPOLLIN | EPOLLOUT | EPOLLET;
                        epoll_ctl(epollfd, EPOLL_CTL_MOD, conn->fd, &cev);
                    }
                    else
                    {
                        // Đã gửi cạn buffer, chỉ cần chờ đọc tiếp
                        cev.events = EPOLLIN | EPOLLET;
                        epoll_ctl(epollfd, EPOLL_CTL_MOD, conn->fd, &cev);
                    }
                }

            next_event:
                continue;
            }
        }
    }
    return 0;
}