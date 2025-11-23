#include "peer.hpp"
#include "msg.hpp"
#include "perfectlink.hpp"
#include <string>
#include <unordered_set>
#include <thread>

static constexpr int MAXLINE = 1024;

void Peer::start()
{
  // read config file
  ifstream configFile(this->configPath_);
  string line;
  int num_messages;
  getline(configFile, line);
  stringstream ss(line);
  ss >> num_messages;
  configFile.close();
  createSocket();
  setNumMessages(num_messages);

  std::thread receiver_thread(&Peer::receiver, this);
  sender();

  receiver_thread.join();
}

unsigned long Peer::myId() { return myId_; }
Parser::Host Peer::myHost() { return myHost_; }
Parser Peer::parser() { return parser_; }
void Peer::setNumMessages(int numMessages)
{
  numMessages_ = numMessages;
}

void Peer::sender()
{

  for (int i = 1; i <= numMessages_; i++)
  {
    Msg msg(MessageType::DATA, this->myId(), to_string(i));
    bebSend(msg);
  }

  while (true)
  {
    // wait for 1s
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    {
      pl_.resend(sockfd_);
    }
  }
}

// best-effort broadcast
void Peer::bebSend(Msg m)
{
  // for all p, addSendlist message
  for (auto &host : parser_.hosts())
  {
    pl_.addSendlist(host, m);
    pl_.send(sockfd_, host, m);
  }
}

void Peer::createSocket()
{
  // initialization of socket
  // File descriptor of a socket
  char buffer[MAXLINE];
  ofstream outputFile(outputPath_);

  // struct sockaddr_in senderaddr, receiveraddr;

  if ((sockfd_ = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
  {
    perror("socket creation failed");
    exit(EXIT_FAILURE);
  }

  struct timeval tv;
  tv.tv_sec = 1;
  tv.tv_usec = 100000;

  if (setsockopt(sockfd_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
  {
    perror("Error setting socket timeout");
    exit(EXIT_FAILURE);
  }

  // bind setting (for receiver)
  struct sockaddr_in myaddr;
  memset(&myaddr, 0, sizeof(myaddr));

  myaddr.sin_family = AF_INET; // IPv4
  myaddr.sin_addr.s_addr = INADDR_ANY;
  myaddr.sin_port = this->myHost().port;

  if (bind(sockfd_, reinterpret_cast<const struct sockaddr *>(&myaddr),
           sizeof(myaddr)) < 0)
  {
    perror("bind failed");
    exit(EXIT_FAILURE);
  }
}

void Peer::receiver()
{
  std::array<char, MAXLINE> buffer;
  struct sockaddr_in senderaddr;
  unordered_set<string> delivered_msgs;
  ofstream outputFile(outputPath_);
  ssize_t n;
  while (true)
  {
    socklen_t len = sizeof(senderaddr);
    n = recvfrom(sockfd_, buffer.data(), buffer.size(), MSG_WAITALL,
                 reinterpret_cast<struct sockaddr *>(&senderaddr), &len);
    if (n < 0)
    {
      // timeout
      continue;
    }
    buffer[n] = '\0';
    string m_serialized = buffer.data();
    // cout << "received payload: " << m_serialized << endl;

    // send ack or process data
    Msg msg{};
    msg.deserialize(m_serialized);
    unsigned long src_id = msg.src_id;
    Parser::Host srcHost = parser_.getHostFromId(src_id);
    pl_.onPacketReceived(sockfd_, myHost_, srcHost, msg);

    // find duplication
    if (msg.type == MessageType::DATA)
    {
      auto it = delivered_msgs.find(m_serialized);
      if (it == delivered_msgs.end())
      {
        // a new message
        delivered_msgs.insert(m_serialized);

        outputFile << "d " << msg.src_id << " " << msg.m << endl;
      }
    }
  }
}
