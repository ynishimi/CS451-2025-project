#pragma once
#include "parser.hpp"

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
  void sender();

private:
  Parser parser_;
  Parser::Host myHost_;
  unsigned long myId_;
};
