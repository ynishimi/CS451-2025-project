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
      : parser_{parser}
  {
    myId_ = parser.srcId();
    myHost_ = parser_.getHostFromId(myId_);
    configPath_ = configPath;
    outputPath_ = outputPath;

    logFile_.open(outputPath_);
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
  void plSend(Parser::Host receiver_host, Msg<LatticePayload> m);
  void sendAck(int sockfd, string m, sockaddr_in senderaddr);

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
  mutex mu_;

  // fifo
  // unordered_map<unsigned long, unsigned int> last_delivered_;
};
