#include <iostream>
#include <set>
#include <algorithm>
#include <unordered_map>
#include <string>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <string.h>

#define MAX_EVENTS 32
#define BUFFER_SIZE 1024
#define IP_ADDR_SIZE 256

int set_nonblock(int fd) {
    int flags;
#if defined (O_NONBLOCK)
    if (-1 == (flags = fcntl(fd, F_GETFL, 0)))
        flags = 0;
    return fcntl(fd, F_GETFL, flags | O_NONBLOCK);
#else
    flags = 1;
    return ioctl(fd, FIOBIO, &flags);
#endif
}

void send_message(const int& CurrentSocket, const std::unordered_map<int, std::string>& PuppetSockets,
                  const std::string& Message) {
    std::string message = PuppetSockets.at(CurrentSocket) + std::string(": ") + Message;
    for (auto it = PuppetSockets.begin(); it != PuppetSockets.end(); ++it) {
        if (it->first != CurrentSocket)
            send(it->first, message.c_str(), message.size(), MSG_NOSIGNAL);
    }
    printf("%d - %s\n", CurrentSocket, message.c_str());
}

int main()
{
    int MasterSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (MasterSocket == -1) {
        perror("cannot create socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in SockAddr;
    SockAddr.sin_family = AF_INET;
    SockAddr.sin_port = htons(12345);
    SockAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(MasterSocket,(struct sockaddr *)&SockAddr, sizeof SockAddr) == -1) {
        perror("bind failed");
        close(MasterSocket);
        exit(EXIT_FAILURE);
    }

    set_nonblock(MasterSocket);

    listen(MasterSocket, SOMAXCONN);

    int EPoll = epoll_create1(0);
    if (EPoll < 0) {
        perror("epoll create failed");
        close(MasterSocket);
        exit(EXIT_FAILURE);
    }

    struct epoll_event Event;
    Event.data.fd = MasterSocket;
    Event.events = EPOLLIN;
    int res = epoll_ctl(EPoll, EPOLL_CTL_ADD, MasterSocket, &Event);
    if (res == -1) {
        perror("epoll_ctl failed");
        close(MasterSocket);
        exit(EXIT_FAILURE);
    }

    std::unordered_map<int, std::string> PuppetSockets;

    while (true) {
        struct epoll_event Events[MAX_EVENTS];
        int N = epoll_wait(EPoll, Events, MAX_EVENTS, -1);
        for (int i = 0; i < N; i++) {
            int CurrentSocket = Events[i].data.fd;
            if (CurrentSocket == MasterSocket) {
                struct sockaddr_in SockAddrAccept;
                socklen_t SockAddrLen = sizeof(SockAddrAccept);
                int PuppetSocket = accept(MasterSocket, (sockaddr *)&SockAddrAccept, &SockAddrLen);

                char IPv4Addr[IP_ADDR_SIZE];
                memset(&IPv4Addr, 0, IP_ADDR_SIZE);
                inet_ntop(SockAddrAccept.sin_family, &SockAddrAccept.sin_addr, IPv4Addr,
                          sizeof(IPv4Addr));

                set_nonblock(PuppetSocket);
                struct epoll_event Event;
                Event.data.fd = PuppetSocket;
                Event.events = EPOLLIN;
                epoll_ctl(EPoll, EPOLL_CTL_ADD, PuppetSocket, &Event);
                PuppetSockets[PuppetSocket] = std::string(IPv4Addr);
                send_message(PuppetSocket, PuppetSockets, "connected!\r\n");
            } else {
                static char Buffer[BUFFER_SIZE];
                memset(&Buffer, 0, BUFFER_SIZE);
                int RecvSize = recv(CurrentSocket, Buffer, BUFFER_SIZE, MSG_NOSIGNAL);
                if ((RecvSize == 0) && (errno != EAGAIN)) {
                    shutdown(CurrentSocket, SHUT_RDWR);
                    close(CurrentSocket);
                    send_message(CurrentSocket, PuppetSockets, "disconnected!\r\n");
                    PuppetSockets.erase(CurrentSocket);
                } else if (RecvSize > 0) {
                    send_message(CurrentSocket, PuppetSockets, Buffer);
                }
            }
        }
    }
    return 0;
}