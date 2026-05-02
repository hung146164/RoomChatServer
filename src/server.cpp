#include "../include/server.h"

Server::Server(int port) : port(port) {
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        throw std::runtime_error("Lỗi tạo socket!");
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        close(server_fd);
        throw std::runtime_error("Lỗi gán địa chỉ!");
    }

    if (listen(server_fd, 3) < 0) {
        close(server_fd);
        throw std::runtime_error("Lỗi lắng nghe!");
    }

    // Khởi tạo tập hợp socket
    FD_ZERO(&master_set);
    FD_SET(server_fd, &master_set);
    max_fd = server_fd;
}

Server::~Server() {
    close(server_fd);
}

void Server::start() {
    std::cout << "Server đa luồng (select) đang chạy tại cổng " << port << "...\n";
    
    while (true) {
        fd_set read_set = master_set;
        // Chờ các sự kiện trên các socket mà không bị chặn hoàn toàn
        int activity = select(max_fd + 1, &read_set, nullptr, nullptr, nullptr);

        if (activity < 0) {
            std::cerr << "Lỗi select!\n";
            break;
        }

        // Duyệt qua tất cả các file descriptor
        for (int i = 0; i <= max_fd; ++i) {
            if (FD_ISSET(i, &read_set)) {
                if (i == server_fd) {
                    // 1. Kết nối mới
                    sockaddr_in client_address;
                    socklen_t client_addrlen = sizeof(client_address);
                    int client_fd = accept(server_fd, (struct sockaddr*)&client_address, &client_addrlen);

                    if (client_fd >= 0) {
                        FD_SET(client_fd, &master_set);
                        if (client_fd > max_fd) {
                            max_fd = client_fd;
                        }
                        std::cout << "Client mới kết nối, fd: " << client_fd << std::endl;
                    }
                } else {
                    // 2. Nhận dữ liệu từ client
                    char buffer[1024] = {0};
                    int bytes_read = recv(i, buffer, sizeof(buffer), 0);

                    if (bytes_read <= 0) {
                        // Kết thúc kết nối hoặc lỗi
                        if (bytes_read == 0) {
                            std::cout << "Client đã ngắt kết nối, fd: " << i << std::endl;
                        } else {
                            std::cerr << "Lỗi đọc dữ liệu, fd: " << i << std::endl;
                        }
                        close(i);
                        FD_CLR(i, &master_set);
                    } else {
                        std::cout << "Nhận từ fd (" << i << "): " << buffer << std::endl;

                        // Gửi phản hồi lại cho client
                        const char* response = "Server da nhan duoc tin nhan cua ban!";
                        send(i, response, strlen(response), 0);
                    }
                }
            }
        }
    }
}