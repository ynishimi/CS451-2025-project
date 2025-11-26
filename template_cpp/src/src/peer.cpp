#include "peer.hpp"
#include "msg.hpp"
#include "perfectlink.hpp"
#include <string>
#include <unordered_set>
#include <thread>

static constexpr int MAXLINE = 1024;

using namespace std;

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
    // newly send the message. relay == src

    Msg msg(MessageType::DATA, myId_, myId_, to_string(i));
    // bebBroadcast(msg);
    urbBroadcast(msg);

    cout << "b " << msg.m << endl;
    logFile_ << "b " << msg.m << endl;
  }

  while (true)
  {
    // wait for 1s
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    {
      tryUrbDeliver();
      pl_.resend(sockfd_);
    }
  }
}

// uniform reliable broadcast
void Peer::urbBroadcast(Msg msg)
{
  urb_.pending.emplace(msg.src_id, msg.m);
  bebBroadcast(msg);
}

void Peer::tryUrbDeliver()
{
  auto it = urb_.pending.begin();
  while (it != urb_.pending.end())
  {
    auto &src_id = it->first;
    const string &m = it->second;

    if (canDeliver(m) && urb_.delivered.count(m) == 0)
    {
      // urbDeliver
      urb_.delivered.emplace(m);
      cout << "d " << src_id << " " << m << endl;
      logFile_ << "d " << src_id << " " << m << endl;

      // todo: delete ack[m]
      // urb_.ack[m]
    }
    it++;
  }
}

bool Peer::canDeliver(const MsgId &mi)
{
  auto it = urb_.ack.find(m);
  auto &acked_peers = it->second;
  return (acked_peers.size() * 2 > parser_.hosts().size());
}

// best-effort broadcast
void Peer::bebBroadcast(Msg msg)
{
  // for all p, addSendlist message
  for (auto &host : parser_.hosts())
  {
    std::cout << "bebBroadcast: " << host.srcId << "msg: " << msg.m << std::endl;
    pl_.addSendlist(host, msg);
    pl_.send(sockfd_, host, msg);
  }
}

void Peer::createSocket()
{
  // initialization of socket
  // File descriptor of a socket
  char buffer[MAXLINE];

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

        // bebDelivered
        urb_.ack[msg.m].emplace(msg.relay_id);

        auto result = urb_.pending.emplace(msg.src_id, msg.m);
        if (result.second)
        {
          bebBroadcast(msg);
        }
      }
    }
  }
}
