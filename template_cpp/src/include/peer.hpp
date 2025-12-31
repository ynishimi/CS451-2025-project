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
  Peer(Parser parser, const char *outputPath, int p, int vs, int ds, vector<proposalSet> proposals)
      //  function<void(const int lattice_shot_num, const LatticePayload &)> broadcastCallback,
      //  function<void(const int lattice_shot_num, const LatticePayload &)> sendCallback)
      : parser_{parser}, proposals_(proposals), p_(p), vs_(vs), ds_(ds)
  {
    myId_ = parser.srcId();
    myHost_ = parser_.getHostFromId(myId_);
    outputPath_ = outputPath;

    logFile_.open(outputPath_);

    decisions_.resize(p_);

    // create lattice proposer/acceptor
    auto num_peer = static_cast<unsigned int>(parser_.hosts().size());
    auto f = num_peer / 2;
    for (int lattice_shot_num = 0; lattice_shot_num < p_; lattice_shot_num++)
    {
      proposers_.emplace_back(lattice_shot_num, f, [this, lattice_shot_num](const LatticePayload &p)
                              { this->latticeBebBroadcast(lattice_shot_num, p); }, [this, lattice_shot_num](const proposalSet &decision)
                              { debugPrint("decision", decision);
                                this->decisions_[lattice_shot_num] = decision; });
      acceptors_.emplace_back(
          lattice_shot_num, [this, lattice_shot_num](const unsigned long dst_id, const LatticePayload &p)
          { this->latticePlSend(dst_id, lattice_shot_num, p); });
    }
  }

public:
  void start();
  unsigned long myId();
  Parser::Host myHost();
  Parser parser();
  // void setReceiverId(int receiverId);
  void setNumMessages(unsigned int numMessages);

  void PrintDecisions();

private:
  void receiver();
  bool receiveAck(int sockfd);
  void sender();
  void createSocket();
  void bebBroadcast(Msg<LatticePayload> m);
  void latticeBebBroadcast(const unsigned int lattice_shot_num, const LatticePayload &p);
  void plSend(Parser::Host receiver_host, Msg<LatticePayload> m);
  void latticePlSend(unsigned long dst_id, const unsigned int lattice_shot_num, const LatticePayload &p);
  unsigned int getPacketCounterAndIncrement();
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
  vector<proposalSet> decisions_;

  int p_;  // num of proposal
  int vs_; // max num of elements in a proposal
  int ds_; // max num of distinct elements across all proposals
};
