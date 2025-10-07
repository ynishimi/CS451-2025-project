#include <chrono>
#include <iostream>
#include <thread>

#include "parser.hpp"
#include "hello.h"
#include <signal.h>

static int SENDER_PORT = 10001;
static int RECEIVER_PORT = 10002;
static int RECEIVER_ID = 3;
static int MAXLINE = 1024;

static void stop(int)
{
  // reset signal handlers to default
  signal(SIGTERM, SIG_DFL);
  signal(SIGINT, SIG_DFL);

  // immediately stop network packet processing
  std::cout << "Immediately stopping network packet processing.\n";

  // write/flush output file if necessary
  std::cout << "Writing output.\n";

  // exit directly from signal handler
  exit(0);
}

int main(int argc, char **argv)
{
  signal(SIGTERM, stop);
  signal(SIGINT, stop);

  // `true` means that a config file is required.
  // Call with `false` if no config file is necessary.
  bool requireConfig = true;

  Parser parser(argc, argv);
  parser.parse();

  hello();
  std::cout << std::endl;

  std::cout << "My PID: " << getpid() << "\n";
  std::cout << "From a new terminal type `kill -SIGINT " << getpid() << "` or `kill -SIGTERM "
            << getpid() << "` to stop processing packets\n\n";

  std::cout << "My ID: " << parser.id() << "\n\n";

  std::cout << "List of resolved hosts is:\n";
  std::cout << "==========================\n";
  auto hosts = parser.hosts();
  for (auto &host : hosts)
  {
    std::cout << host.id << "\n";
    std::cout << "Human-readable IP: " << host.ipReadable() << "\n";
    std::cout << "Machine-readable IP: " << host.ip << "\n";
    std::cout << "Human-readbale Port: " << host.portReadable() << "\n";
    std::cout << "Machine-readbale Port: " << host.port << "\n";
    std::cout << "\n";
  }
  std::cout << "\n";

  std::cout << "Path to output:\n";
  std::cout << "===============\n";
  std::cout << parser.outputPath() << "\n\n";

  std::cout << "Path to config:\n";
  std::cout << "===============\n";
  std::cout << parser.configPath() << "\n\n";

  std::cout << "Doing some initialization...\n\n";

  std::cout << "Broadcasting and delivering messages...\n\n";

  const int receiver_id = RECEIVER_ID;

  if (parser.id() == receiver_id)
  {
    // receiver
    receiver();
  }
  else
  {
    // sender
    sender();
  }

  // After a process finishes broadcasting,
  // it waits forever for the delivery of messages.
  while (true)
  {
    std::this_thread::sleep_for(std::chrono::hours(1));
  }

  return 0;
}

void receiver()
{
  // File descriptor of a socket
  int sockfd;
  // std::vector<char> buffer;
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
  if (bind(sockfd, (const struct sockaddr *)&receiveraddr,
           sizeof(receiveraddr)) < 0)
  {
    perror("bind failed");
    exit(EXIT_FAILURE);
  }

  socklen_t len;
  int n;

  // len = sizeof(receiveraddr); // len is value/result

  n = recvfrom(sockfd, (char *)buffer, MAXLINE,
               MSG_WAITALL, (struct sockaddr *)&senderaddr,
               &len);
  buffer[n] = '\0';
  printf("Client : %s\n", buffer);
  // sendto(sockfd, (const char *)hello, strlen(hello),
  //        0, (const struct sockaddr *)&receiveraddr,
  //        len);
  std::cout << "sent message: " << hello << std::endl;
}

void sender()
{
  // File descriptor of a socket
  int sockfd;
  char buffer[MAXLINE];
  const char *hello = "Hello from sender";
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

  // // Bind the socket with the sender address
  // if (bind(sockfd, (const struct sockaddr *)&senderaddr,
  //          sizeof(senderaddr)) < 0)
  // {
  //   perror("bind failed");
  //   exit(EXIT_FAILURE);
  // }

  socklen_t len;
  // int n;

  len = sizeof(receiveraddr); // len is value/result

  // n = recvfrom(sockfd, (char *)buffer, MAXLINE,
  //              MSG_WAITALL, (struct sockaddr *)&cliaddr,
  //              &len);
  // buffer[n] = '\0';
  // printf("Client : %s\n", buffer);
  sendto(sockfd, (const char *)hello, strlen(hello),
         0, (const struct sockaddr *)&receiveraddr,
         len);
  std::cout << "sent message: " << hello << std::endl;
}