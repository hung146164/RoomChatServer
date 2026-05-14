#include "../header/networkcommon.h"

int NUM_BOTS = 100;
int PORT = 8080;
char *SERVER_IP = "127.0.0.1";

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
void BotTask(int botId)
{
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(PORT);
    serverAddress.sin_addr.s_addr = inet_addr(SERVER_IP);

    if (connect(clientSocket, (sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
    {
        std::cout << "Connect Failed" << '\n';
        close(clientSocket);
        return;
    }
    for (int i = 1; i <= 5; ++i)
    {
        std::string msg = "Hello from Bot " + std::to_string(botId) + " - Message " + std::to_string(i);
        auto start = std::chrono::high_resolution_clock::now();
        int byteSend = send(clientSocket, msg.c_str(), msg.length(), 0);

        if (byteSend <= 0)
        {
            std::cerr << "[Bot " << botId << "] Connection lost.\n";
            break;
        }

        char buffer[1024];
        int bytesReceived = recv(clientSocket, buffer, 1024, 0);
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        std::cout << "[Bot " << botId << "] Sent " << byteSend
                  << " bytes. Latency: " << duration.count() << " us\n";

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    close(clientSocket);
}
int main()
{
    std::cout << "Start Test entor your number bot: " << '\n';
    std::cin >> NUM_BOTS;

    std::vector<std::thread> botThreads;

    for (int i = 0; i < NUM_BOTS; i++)
    {
        botThreads.push_back(std::thread(BotTask, i));
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    for (auto &t : botThreads)
    {
        if (t.joinable())
        {
            t.join();
        }
    }
    std::cout << "Complete" << '\n';
    return 0;
}