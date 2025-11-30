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

    // this case to_string(seq_id) == m
    Msg msg(MessageType::DATA, myId_, i, myId_, to_string(i));
    // bebBroadcast(msg);
    urbBroadcast(msg);

    // cout << "b " << msg.m << endl;
    logFile_ << "b " << msg.m << endl;
  }

  while (true)
  {
    // wait for 1s
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    {
      pl_.resend(sockfd_);
    }
  }
}

// uniform reliable broadcast
void Peer::urbBroadcast(Msg msg)
{
  urb_.pending[msg.src_id].emplace(msg);
  bebBroadcast(msg);
}

void Peer::tryUrbDeliver()
{
  // lock_guard<mutex> lock(mu_);

  // cout << "tryUrbDeliver()" << endl;
  for (auto it_map = urb_.pending.begin(); it_map != urb_.pending.end(); it_map++)
  {
    unsigned long src_id = it_map->first;
    std::set<Msg> &msgs = it_map->second;

    // iterates through messages
    auto it_msg = msgs.begin();
    while (it_msg != msgs.end())
    {
      const MsgId msgId = MsgId(src_id, it_msg->seq_id);
      // FIFODeliver
      if (!canFIFODeliver(msgId))
      {
        break;
      }

      // urbDeliver
      if (urb_.delivered.count(msgId) == 0 && canDeliver(msgId))
      {
        urb_.delivered.emplace(msgId);
        // cout << "d " << msgId.first << " " << it_msg->m << endl;
        logFile_ << "d " << msgId.first << " " << it_msg->m << endl;

        last_delivered_[msgId.first]++;

        // delete ack[msgId]
        urb_.ack.erase(msgId);
        // delete pending[Msg]
        it_msg = msgs.erase(it_msg);
        continue;
      }
      else
      {
        break;
      }
    }
  }
}

bool Peer::canFIFODeliver(const MsgId &mi)
{
  auto last_delivered_seq = last_delivered_[mi.first];
  if (last_delivered_seq + 1 == mi.second)
  {
    return true;
  }
  else
  {
    // debug
    // cout << "debug: " << "FIFODeliver not successful" << endl;
    return false;
  }
}

bool Peer::canDeliver(const MsgId &mi)
{
  if (urb_.ack.count(mi) == 0)
  {
    cout << "mi not found" << endl;
    return false;
  }
  auto it = urb_.ack.find(mi);
  auto &acked_peers = it->second;
  // cout << "acked_peers: " << acked_peers.size() << ", parser_.hosts: " << parser_.hosts().size() << endl;
  if (acked_peers.size() * 2 > parser_.hosts().size())
  {
    return true;
  }
  else
  {
    return false;
  }
}

// best-effort broadcast
void Peer::bebBroadcast(Msg msg)
{
  // for all p, addSendlist message
  for (auto &host : parser_.hosts())
  {
    // std::cout << "bebBroadcast: " << host.srcId << "msg: " << msg.m << std::endl;
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
  // set<MsgId> delivered_msgs;
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
    unsigned long relay_id = msg.relay_id;
    unsigned int seq_id = msg.seq_id;
    MsgId msgId = MsgId(src_id, seq_id);
    Parser::Host relayHost = parser_.getHostFromId(relay_id);
    pl_.onPacketReceived(sockfd_, myHost_, relayHost, msg);

    if (msg.type == MessageType::DATA)
    {
      lock_guard<mutex> lock(mu_);

      // do not ack/broadcast msgs which are already delivered
      if (urb_.delivered.count(msgId) > 0)
      {
        continue;
      }

      urb_.ack[msgId].emplace(msg.relay_id);

      auto result = urb_.pending[msg.src_id].emplace(msg);
      auto not_pending = result.second;
      if (not_pending)
      {
        msg.relay_id = myId_;
        bebBroadcast(msg);
      }
      // tries to deliver the message
      tryUrbDeliver();
    }
  }
}
