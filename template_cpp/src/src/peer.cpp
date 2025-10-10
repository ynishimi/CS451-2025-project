#include "peer.hpp"

#include <string>
#include <unordered_set>

static constexpr int MAXLINE = 1024;

void Peer::start()
{
  // read config file
  ifstream configFile(this->configPath_);
  string line;
  int num_messages, receiver_id;
  getline(configFile, line);
  stringstream ss(line);
  ss >> num_messages >> receiver_id;
  configFile.close();

  setNumMessages(num_messages);
  setReceiverId(receiver_id);

  cout << receiver_id << endl;
  if (myId() == static_cast<unsigned long>(receiver_id))
  {
    receiver();
  }
  else
  {
    sender();
  }
}

unsigned long Peer::myId() { return myId_; }
Parser::Host Peer::myHost() { return myHost_; }
Parser Peer::parser() { return parser_; }
void Peer::setReceiverId(int receiverId)
{
  receiverId_ = receiverId;
}
void Peer::setNumMessages(int numMessages)
{
  numMessages_ = numMessages;
}

void Peer::sender()
{
  // File descriptor of a socket
  int sockfd;
  char buffer[MAXLINE];
  // std::string hello = "Hello from sender " + std::to_string(parser_.id());
  ofstream outputFile(outputPath_);

  struct sockaddr_in senderaddr, receiveraddr;

  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
  {
    perror("socket creation failed");
    exit(EXIT_FAILURE);
  }

  struct timeval tv;
  tv.tv_sec = 1;
  tv.tv_usec = 100000;

  if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
  {
    perror("Error setting socket timeout");
    exit(EXIT_FAILURE);
  }
  memset(&receiveraddr, 0, sizeof(receiveraddr));

  Parser::Host receiver_host = this->parser().getHostFromId(receiverId_);
  receiveraddr.sin_family = AF_INET; // IPv4
  receiveraddr.sin_addr.s_addr = receiver_host.ip;
  receiveraddr.sin_port = receiver_host.port;

  socklen_t len = sizeof(receiveraddr); // len is value/result

  for (int i = 0; i < numMessages_; i++)
  {
    bool acked = false;
    std::string m = std::to_string(i + 1);
    while (!acked)
    {
      sendto(sockfd, m.data(), m.size(), 0,
             reinterpret_cast<const struct sockaddr *>(&receiveraddr), len);
      std::cout << "sent message: " << m << std::endl;

      // receive ack
      acked = receiveAck(sockfd);
      std::cout << "acked: " << acked << std::endl;
    }
    outputFile << "b " << m << endl;
  }
}

bool Peer::receiveAck(int sockfd)
{
  std::array<char, MAXLINE> buffer;
  struct sockaddr_in senderaddr, receiveraddr;

  memset(&senderaddr, 0, sizeof(senderaddr));
  memset(&receiveraddr, 0, sizeof(receiveraddr));

  // Filling sender information
  Parser::Host my_host = this->myHost();
  receiveraddr.sin_family = AF_INET; // IPv4
  receiveraddr.sin_addr.s_addr = my_host.ip;
  receiveraddr.sin_port = my_host.port;

  socklen_t len = sizeof(senderaddr);
  ssize_t n;
  n = recvfrom(sockfd, buffer.data(), buffer.size(), 0,
               reinterpret_cast<struct sockaddr *>(&senderaddr), &len);
  if (n < 0)
  {
    // recv timeout
    return false;
  }
  buffer[n] = '\0';
  std::cout << "Received ack :" << buffer.data() << std::endl;
  return true;
}

void Peer::receiver()
{
  // File descriptor of a socket
  int sockfd;
  std::array<char, MAXLINE> buffer;
  struct sockaddr_in senderaddr, receiveraddr;
  unordered_set<string> messages;
  ofstream outputFile(outputPath_);

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
  receiveraddr.sin_port = this->myHost().port;

  // Bind the socket with the sender address
  if (bind(sockfd, reinterpret_cast<const struct sockaddr *>(&receiveraddr),
           sizeof(receiveraddr)) < 0)
  {
    perror("bind failed");
    exit(EXIT_FAILURE);
  }

  socklen_t len = sizeof(senderaddr);
  ssize_t n;
  while (true)
  {
    n = recvfrom(sockfd, buffer.data(), buffer.size(), MSG_WAITALL,
                 reinterpret_cast<struct sockaddr *>(&senderaddr), &len);
    if (n < 0)
    {
      return;
    }
    buffer[n] = '\0';
    std::cout << "Received from " << senderaddr.sin_addr.s_addr << ": " << buffer.data() << std::endl;
    string m = buffer.data();
    auto it = messages.find(m);
    if (it == messages.end())
    {
      messages.insert(m);
      sendAck(sockfd, m, senderaddr);
      Parser::Host host = parser_.getHostFromAddr(senderaddr);
      outputFile << "d " << host.id << " " << m << endl;
    }
  }
}

void Peer::sendAck(int sockfd, string m, sockaddr_in senderaddr)
{
  socklen_t len = sizeof(senderaddr);
  sendto(sockfd, m.data(), m.size(), 0,
         reinterpret_cast<const struct sockaddr *>(&senderaddr), len);
  std::cout << "Sent ack: " << m << std::endl;
}