#include "../include/server.h"

int main() {
    try {
        Server myServer(8080);
        myServer.start();
    } catch (const std::exception& e) {
        std::cerr << "Lỗi: " << e.what() << std::endl;
        return -1;
    }
    return 0;
}