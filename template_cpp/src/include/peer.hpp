#pragma once
#include "parser.hpp"

class Peer
{
public:
  Peer(Parser parser)
      : parser{parser} {}

public:
  void start();

private:
  void receiver();
  void sender();

private:
  Parser parser;
};
