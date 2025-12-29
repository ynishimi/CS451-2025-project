#pragma once

#include <set>
#include <map>
#include <unordered_map>

#include "parser.hpp"
#include "msg.hpp"
#include "perfectlink.hpp"
#include "lattice.hpp"

using namespace std;

// (src_id, seq_id)
using MsgId = pair<unsigned long, unsigned int>;

class Peer
{
public:
  Peer(Parser parser, const char *configPath, const char *outputPath)
      //  function<void(const int lattice_shot_num, const LatticePayload &)> broadcastCallback,
      //  function<void(const int lattice_shot_num, const LatticePayload &)> sendCallback)
      : parser_{parser}
  {
    myId_ = parser.srcId();
    myHost_ = parser_.getHostFromId(myId_);
    configPath_ = configPath;
    outputPath_ = outputPath;

    logFile_.open(outputPath_);

    // create lattice proposer/acceptor
    int num_peer = 2;
    int f = num_peer / 2;
    int lattice_shot_num = 0;
    for (int i = 0; i < 1; i++)
    {
      proposers_.emplace_back(lattice_shot_num, f, [&](const LatticePayload &p)
                              { this->latticeBebBroadcast(i, p); });
      acceptors_.emplace_back(lattice_shot_num, f, [&](const unsigned long dst_id, const LatticePayload &p)
                              { this->latticePlSend(dst_id, i, p); });
    }

    // // create LatticeProposer
    // int num_peer = 2;
    // LatticeProposer proposer(num_peer, [this](const LatticePayload &p)
    //                          { this->latticeBebBroadcast(p); });

    // //  create LatticeAcceptor
    // LatticeAcceptor acceptor([this](const LatticePayload &p)
    //                          { this->latticePlSend(p); });
  }

public:
  void start();
  unsigned long myId();
  Parser::Host myHost();
  Parser parser();
  // void setReceiverId(int receiverId);
  void setNumMessages(unsigned int numMessages);

private:
  void receiver();
  bool receiveAck(int sockfd);
  void sender();
  void createSocket();
  // void urbBroadcast(Msg<string> m);
  // void tryUrbDeliver();
  // bool canDeliver(const MsgId &mi);
  // bool canFIFODeliver(const MsgId &mi);
  void bebBroadcast(Msg<LatticePayload> m);
  void latticeBebBroadcast(const int lattice_shot_num, const LatticePayload &p);
  void plSend(Parser::Host receiver_host, Msg<LatticePayload> m);
  void latticePlSend(unsigned long dst_id, const int lattice_shot_num, const LatticePayload &p);
  int getPacketCounterAndIncrement();
  void sendAck(int sockfd, string m, sockaddr_in senderaddr);
  void latticeHandler(unsigned long src_id, const int lattice_shot_num, const LatticePayload &p);

private:
  Parser parser_;
  Parser::Host myHost_;
  unsigned long myId_;
  const char *configPath_;
  const char *outputPath_;
  // int receiverId_;
  unsigned int numMessages_;
  int sockfd_;
  PerfectLink pl_;
  ofstream logFile_;
  // packet_counter_ is used as ID of each packet
  int packet_counter_ = 0;
  mutex packet_counter_mu_;

  // lattice agreement
  vector<LatticeProposer> proposers_;
  vector<LatticeAcceptor> acceptors_;
  vector<proposalSet> proposals_;

  // struct Urb
  // {
  //   // todo: maybe I should use seq_ids for keys

  //   // (MsgId)
  //   set<MsgId> delivered;
  //   // (src_id, Msg)
  //   map<unsigned long, std::set<Msg<T>>> pending;
  //   // (MsgId, relay_id)
  //   map<MsgId, set<unsigned long>> ack;
  // };
  // Urb urb_;

  // fifo
  // unordered_map<unsigned long, unsigned int> last_delivered_;
};
