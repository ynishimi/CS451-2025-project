#include "peer.hpp"
#include "msg.hpp"
#include "perfectlink.hpp"
#include <string>
#include <unordered_set>
#include <thread>
#include "common.hpp"

static constexpr int MAXLINE = 65536;

using namespace std;

void Peer::start()
{
  createSocket();

  std::thread receiver_thread(&Peer::receiver, this);
  sender();

  receiver_thread.join();
}

unsigned long Peer::myId() { return myId_; }
Parser::Host Peer::myHost() { return myHost_; }
Parser Peer::parser() { return parser_; }

void Peer::sender()
{
  for (int i = 0; i < p_; i++)
  {
    proposers_[i].Propose(proposals_[i]);
  }

  while (true)
  {
    // wait for some time
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    {
      pl_.resend(sockfd_);
    }
  }
}

void Peer::PrintDecisions()
{
  for (auto &decision : decisions_)
  {
    logFile_ << decision << endl;
  }
}

// best-effort broadcast
void Peer::bebBroadcast(Msg<LatticePayload> msg)
{
  // for all p, addSendlist message
  for (auto &host : parser_.hosts())
  {
    // std::cout << "bebBroadcast: " << host.srcId << "msg: " << msg.m << std::endl;
    pl_.addSendlist(host, msg);
    pl_.send(sockfd_, host, msg);
  }
}

// broadcasts latticePayload
void Peer::latticeBebBroadcast(const unsigned int lattice_shot_num, const LatticePayload &p)
{
  // create msg and broadcast it
  // MessageType type, unsigned long src_id, unsigned int seq_id, unsigned long relay_id, unsigned int lattice_shot_num, const T &p

  debugPrint("broadcast", p);

  bebBroadcast(Msg<LatticePayload>{
      MessageType::DATA,
      myId_,
      getPacketCounterAndIncrement(),
      myId_,
      lattice_shot_num,
      p});
}

void Peer::latticePlSend(const unsigned long dst_id, const unsigned int lattice_shot_num, const LatticePayload &p)
{
  Msg<LatticePayload> msg{
      MessageType::DATA,
      myId_,
      getPacketCounterAndIncrement(),
      myId_,
      lattice_shot_num,
      p};

  auto dst_host = parser_.getHostFromId(dst_id);
  pl_.addSendlist(dst_host, msg);
  pl_.send(sockfd_, dst_host, msg);
}

unsigned int Peer::getPacketCounterAndIncrement()
{
  lock_guard<mutex> lock(packet_counter_mu_);
  int cur_pc = this->packet_counter_;
  this->packet_counter_++;
  return cur_pc;
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

  if (::bind(sockfd_, reinterpret_cast<const struct sockaddr *>(&myaddr),
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
  set<MsgId> delivered_msgs;
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
    buffer[static_cast<size_t>(n)] = '\0';
    string m_serialized = buffer.data();
    // cout << "received payload: " << m_serialized << endl;

    // send ack or process data
    Msg<LatticePayload> msg{};
    msg.deserialize(m_serialized);
    unsigned long src_id = msg.src_id;
    unsigned long relay_id = msg.relay_id;
    unsigned int seq_id = msg.seq_id;
    MsgId msgId = MsgId(src_id, seq_id);
    Parser::Host relayHost = parser_.getHostFromId(relay_id);
    pl_.onPacketReceived(sockfd_, myHost_, relayHost, msg);

    if (msg.type == MessageType::DATA)
    {
      if (delivered_msgs.find(msgId) == delivered_msgs.end())
      {
        delivered_msgs.insert(msgId);
        // pass the payload to lattice proposer/acceptor
        latticeHandler(msg.src_id, msg.lattice_shot_num, msg.payload);
      }
    }
  }
}

// latticeHandler
void Peer::latticeHandler(unsigned long src_id, const int lattice_shot_num, const LatticePayload &p)
{
  debugPrint("received", p);
  if (p.type == LatticeMessageType::PROPOSAL)
  {
    // acceptor.Receive(p);
    acceptors_[lattice_shot_num].Receive(src_id, p);
  }
  else
  {
    proposers_[lattice_shot_num].Receive(p);
  }
}
