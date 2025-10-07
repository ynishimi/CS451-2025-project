#include "peer.hpp"

#include <string>

static constexpr int SENDER_PORT = 10001;
static constexpr int RECEIVER_PORT = 10002;
static constexpr int RECEIVER_ID = 3;
static constexpr int MAXLINE = 1024;

void Peer::start()
{
    if (parser.id() == RECEIVER_ID)
    {
        receiver();
    }
    else
    {
        sender();
    }
}

void Peer::receiver()
{
    // File descriptor of a socket
    int sockfd;
    std::array<char, MAXLINE> buffer;
    const char *hello = "Hello from receiver";
    struct sockaddr_in senderaddr, receiveraddr;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }
    memset(&senderaddr, 0, sizeof(senderaddr));
    memset(&receiveraddr, 0, sizeof(receiveraddr));

    // Filling sender information
    receiveraddr.sin_family = AF_INET; // IPv4
    receiveraddr.sin_addr.s_addr = INADDR_ANY;
    receiveraddr.sin_port = htons(RECEIVER_PORT);

    // Bind the socket with the sender address
    if (bind(sockfd, reinterpret_cast<const struct sockaddr *>(&receiveraddr),
             sizeof(receiveraddr)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    socklen_t len;
    ssize_t n;

    n = recvfrom(sockfd, buffer.data(), buffer.size(),
                 MSG_WAITALL, reinterpret_cast<struct sockaddr *>(&senderaddr),
                 &len);
    buffer[n] = '\0';
    std::cout << "Client :" << buffer.data() << std::endl;
    std::cout << "sent message: " << hello << std::endl;
}

void Peer::sender()
{
    // File descriptor of a socket
    int sockfd;
    char buffer[MAXLINE];
    std::string hello = "Hello from sender " + std::to_string(parser.id());

    struct sockaddr_in senderaddr, receiveraddr;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }
    memset(&senderaddr, 0, sizeof(senderaddr));
    memset(&receiveraddr, 0, sizeof(receiveraddr));

    // Filling sender information
    senderaddr.sin_family = AF_INET; // IPv4
    senderaddr.sin_addr.s_addr = INADDR_ANY;
    senderaddr.sin_port = htons(SENDER_PORT);
    receiveraddr.sin_family = AF_INET; // IPv4
    receiveraddr.sin_addr.s_addr = INADDR_ANY;
    receiveraddr.sin_port = htons(RECEIVER_PORT);

    socklen_t len;

    len = sizeof(receiveraddr); // len is value/result

    sendto(sockfd, hello.data(), hello.size(),
           0, reinterpret_cast<const struct sockaddr *>(&receiveraddr),
           len);
    std::cout << "sent message: " << hello << std::endl;
}