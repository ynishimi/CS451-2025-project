#pragma once
#include "parser.hpp"

using namespace std;

class Peer
{
public:
  Peer(Parser parser)
      : parser_{parser}
  {
    myId_ = parser.id();

    myHost_ = parser_.getHostFromId(myId_);
  }

public:
  void start();
  unsigned long myId();
  Parser::Host myHost();
  Parser parser();

private:
  void receiver();
  bool receiveAck(int sockfd);
  void sender();
  void sendAck(int sockfd, string m, sockaddr_in senderaddr);

private:
  Parser parser_;
  Parser::Host myHost_;
  unsigned long myId_;
};
