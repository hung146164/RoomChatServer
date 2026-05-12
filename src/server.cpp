#include "../header/networkcommon.h"

// network config
const int port = 8080;

// room config
const int listenSize = 5;

//
struct ClientDetail
{
    int32_t serverfd;
    int32_t currentClientfd;
    std::vector<int32_t> clientfdList;
    ClientDetail()
    {
        serverfd = -1;
        currentClientfd = -1;
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

    fd_set readfd;
    int maxfd;
    int activity;
    while (true)
    {
        FD_ZERO(&readfd);
        // lắng nghe kết nối mới
        FD_SET(clientdetail->serverfd, &readfd);

        maxfd = clientdetail->serverfd;

        // lắng nghe kết nối cũ
        for (auto sd : clientdetail->clientfdList)
        {
            FD_SET(sd, &readfd);
            if (maxfd < sd)
            {
                maxfd = sd;
            }
        }
        activity = select(maxfd + 1, &readfd, NULL, NULL, NULL);

        if (activity < 0)
        {
            continue;
        }
        if (FD_ISSET(clientdetail->serverfd, &readfd))
        {
            clientdetail->currentClientfd = accept(clientdetail->serverfd, NULL, NULL);
            if (clientdetail->currentClientfd >= 0)
            {
                clientdetail->clientfdList.push_back(clientdetail->currentClientfd);
                std::cout << "New client, socket fd id: " << clientdetail->currentClientfd << '\n';
            }
            else
            {
                std::cerr << "Accept failed!\n";
            }
        }

        char message[1024];
        for (int i = clientdetail->clientfdList.size() - 1; i >= 0; i--)
        {
            int sd = clientdetail->clientfdList[i];

            if (FD_ISSET(sd, &readfd))
            {
                int valread = read(sd, message, 1024);
                if (valread == 0)
                {
                    std::cout << sd << " Disconnected!" << '\n';
                    close(sd);
                    clientdetail->clientfdList.erase(clientdetail->clientfdList.begin() + i);
                    continue;
                }
                else if (valread < 0)
                {

                    std::cerr << "Read error on fd " << sd << ". Force disconnect.\n";
                    close(sd);
                    clientdetail->clientfdList.erase(clientdetail->clientfdList.begin() + i);
                }
                else
                {
                    message[valread] = '\0';
                    std::cout << "Message received from " << sd << ": " << message << '\n';
                }
            }
        }
    }
    close(clientdetail->serverfd);
    for (auto sd : clientdetail->clientfdList)
    {
        close(sd);
    }
    delete clientdetail;

    return 0;
}