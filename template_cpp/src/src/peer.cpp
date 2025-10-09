#include "peer.hpp"

#include <string>

static constexpr int RECEIVER_ID = 3;
static constexpr int MAXLINE = 1024;
static constexpr int NUM_MESSAGE = 10;
void Peer::start() {
  if (myId() == RECEIVER_ID) {
    receiver();
  } else {
    sender();
  }
}

unsigned long Peer::myId() { return myId_; }
Parser::Host Peer::myHost() { return myHost_; }
Parser Peer::parser() { return parser_; }

void Peer::sender() {
  // File descriptor of a socket
  int sockfd;
  char buffer[MAXLINE];
  std::string hello = "Hello from sender " + std::to_string(parser_.id());

  struct sockaddr_in senderaddr, receiveraddr;

  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("socket creation failed");
    exit(EXIT_FAILURE);
  }

  struct timeval tv;
  tv.tv_sec = 1;
  tv.tv_usec = 100000;

  if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
    perror("Error setting socket timeout");
    exit(EXIT_FAILURE);
  }
  // memset(&senderaddr, 0, sizeof(senderaddr));
  memset(&receiveraddr, 0, sizeof(receiveraddr));

  receiveraddr.sin_family = AF_INET; // IPv4
  receiveraddr.sin_addr.s_addr = INADDR_ANY;
  receiveraddr.sin_port = this->parser().getHostFromId(RECEIVER_ID).port;

  socklen_t len = sizeof(receiveraddr); // len is value/result

  for (int i = 0; i < NUM_MESSAGE; i++) {
    bool acked = false;
    while (!acked) {
      std::string m = std::to_string(i + 1);
      sendto(sockfd, m.data(), m.size(), 0,
             reinterpret_cast<const struct sockaddr *>(&receiveraddr), len);
      std::cout << "sent message: " << m << std::endl;

      // receive ack
      bool is_acked = receiveAck(sockfd);
    }
  }
}

bool Peer::receiveAck(int sockfd) {
  std::array<char, MAXLINE> buffer;
  struct sockaddr_in senderaddr, receiveraddr;

  memset(&senderaddr, 0, sizeof(senderaddr));
  memset(&receiveraddr, 0, sizeof(receiveraddr));

  // Filling sender information
  receiveraddr.sin_family = AF_INET; // IPv4
  receiveraddr.sin_addr.s_addr = INADDR_ANY;
  receiveraddr.sin_port = this->myHost().port;

  socklen_t len = sizeof(senderaddr);
  ssize_t n;
  n = recvfrom(sockfd, buffer.data(), buffer.size(), MSG_WAITALL,
               reinterpret_cast<struct sockaddr *>(&senderaddr), &len);
  if (n < 0) {
    // recv timeout
    return false;
  }
  buffer[n] = '\0';
  std::cout << "Client :" << buffer.data() << std::endl;
  return true;
}

void Peer::receiver() {
  // File descriptor of a socket
  int sockfd;
  std::array<char, MAXLINE> buffer;
  struct sockaddr_in senderaddr, receiveraddr;

  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("socket creation failed");
    exit(EXIT_FAILURE);
  }
  memset(&senderaddr, 0, sizeof(senderaddr));
  memset(&receiveraddr, 0, sizeof(receiveraddr));

  // Filling sender information
  receiveraddr.sin_family = AF_INET; // IPv4
  receiveraddr.sin_addr.s_addr = INADDR_ANY;
  receiveraddr.sin_port = this->myHost().port;

  // Bind the socket with the sender address
  if (bind(sockfd, reinterpret_cast<const struct sockaddr *>(&receiveraddr),
           sizeof(receiveraddr)) < 0) {
    perror("bind failed");
    exit(EXIT_FAILURE);
  }

  socklen_t len = sizeof(senderaddr);
  ;
  ssize_t n;
  n = recvfrom(sockfd, buffer.data(), buffer.size(), MSG_WAITALL,
               reinterpret_cast<struct sockaddr *>(&senderaddr), &len);
  buffer[n] = '\0';
  std::cout << "Client :" << buffer.data() << std::endl;
}
