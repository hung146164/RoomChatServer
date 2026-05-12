#include "../header/networkcommon.h"

int main()
{
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");

    const char *message = "hello world";
    if (connect(clientSocket, (sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
    {
        std::cout << "Connect Failed" << '\n';
        close(clientSocket);
        return 1;
    }
    while (true)
    {
        char s[1024];

        std::cout << "What you want to sent: " << '\n';
        std::cin.getline(s, 1024);

        int byteSend = send(clientSocket, s, strlen(s), 0);
        if (byteSend == 0)
            break;
        std::cout << "Send success " << byteSend << "Byte" << '\n';
    }
    close(clientSocket);
}