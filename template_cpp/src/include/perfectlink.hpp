#pragma once
#include <mutex>
#include <tuple>
#include <vector>

#include "parser.hpp"
#include "msg.hpp"

using namespace std;
// (src_id, seq_id)
using MsgId = pair<unsigned long, unsigned int>;
std::ostream &operator<<(std::ostream &os, const MsgId &id);

class PerfectLink
{
public:
    PerfectLink() {}
    void addSendlist(Parser::Host dest, Msg m);
    void send(int sockfd, Parser::Host dest, Msg m);
    void resend(int sockfd);
    void onPacketReceived(int sockfd, Parser::Host myHost, Parser::Host srcHost, Msg msg);

private:
    void sendAck(int sockfd, Parser::Host myHost, Msg msg);

private:
    bool acked_;
    mutex mu_;
    std::vector<tuple<Parser::Host, Msg>> sendlist_;
};