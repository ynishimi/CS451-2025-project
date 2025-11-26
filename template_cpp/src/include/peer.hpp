#pragma once
#include <set>
#include <map>

#include "parser.hpp"
#include "msg.hpp"
#include "perfectlink.hpp"

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
  void setNumMessages(int numMessages);

private:
  void receiver();
  bool receiveAck(int sockfd);
  void sender();
  void createSocket();
  void urbBroadcast(Msg m);
  void tryUrbDeliver();
  bool canDeliver(const MsgId &mi);
  void bebBroadcast(Msg m);
  void plSend(Parser::Host receiver_host, Msg m);
  void sendAck(int sockfd, string m, sockaddr_in senderaddr);

private:
  Parser parser_;
  Parser::Host myHost_;
  unsigned long myId_;
  const char *configPath_;
  const char *outputPath_;
  // int receiverId_;
  int numMessages_;
  int sockfd_;
  PerfectLink pl_;
  ofstream logFile_;

  struct Urb
  {
    // todo: maybe I should use seq_ids for keys

    // (MsgId)
    set<MsgId> delivered;
    // (Msg)
    set<Msg> pending;
    // (MsgId, relay_id)
    map<MsgId, set<unsigned long>> ack;
  };
  Urb urb_;
};
