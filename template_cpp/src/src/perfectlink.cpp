#include "perfectlink.hpp"
using namespace std;

// output of MsgID
std::ostream &operator<<(std::ostream &os, const MsgId &id)
{
    os << "src_id=" << id.first << ", seq_id=" << id.second;
    return os;
}

// Put the message to outbox_. When
void PerfectLink::addSendlist(Parser::Host dest, Msg<LatticePayload> msg)
{
    lock_guard<mutex> lock(mu_);
    sendlist_.push_back(make_tuple(dest, msg));
}

void PerfectLink::send(int sockfd, Parser::Host dest, Msg<LatticePayload> msg)
{
    // receiver-specific setting
    struct sockaddr_in destAddr;

    destAddr.sin_family = AF_INET; // IPv4
    destAddr.sin_addr.s_addr = dest.ip;
    destAddr.sin_port = dest.port;

    socklen_t len = sizeof(destAddr); // len is value/result

    bool acked = false;
    string serialized_msg = msg.serialize();
    sendto(sockfd, serialized_msg.data(), serialized_msg.size(), 0,
           reinterpret_cast<const struct sockaddr *>(&destAddr), len);
    // std::cout << "sent message: " << serialized_msg << std::endl;
}

// resend unacked messages
void PerfectLink::resend(int sockfd)
{
    // std::cout << "resend message(s)" << std::endl;
    lock_guard<mutex> lock(mu_);
    for (auto &item : sendlist_)
    {
        send(sockfd, get<0>(item), get<1>(item));
    }
}

void PerfectLink::onPacketReceived(int sockfd, Parser::Host myHost, Parser::Host P2PsrcHost, Msg<LatticePayload> msg)
{
    lock_guard<mutex> lock(mu_);

    if (msg.type == MessageType::ACK)
    {
        MsgId ackMsgId(msg.src_id, msg.seq_id);
        for (auto it = sendlist_.begin(); it != sendlist_.end();)
        {
            Parser::Host &waitingDest = get<0>(*it);
            Msg<LatticePayload> &pendingMsg = get<1>(*it);
            MsgId pendingId(pendingMsg.src_id, pendingMsg.seq_id);

            // delete item from sendlist
            if (waitingDest.srcId == P2PsrcHost.srcId && pendingId == ackMsgId)
            {
                it = sendlist_.erase(it);
                // std::cout << "ACK Received for seq " << msg.m << ". Removed from list." << std::endl;
            }
            else
            {
                ++it;
            }
        }
    }
    else if (msg.type == MessageType::DATA)
    {
        // send ackMsg. relay == src
        Msg<LatticePayload> ackMsg(MessageType::ACK, msg.src_id, msg.seq_id, myHost.srcId, msg.payload);

        sendAck(sockfd, P2PsrcHost, ackMsg);
    }
}

void PerfectLink::sendAck(int sockfd, Parser::Host P2PsrcHost, Msg<LatticePayload> ackMsg)
{
    send(sockfd, P2PsrcHost, ackMsg);
    // std::cout << "Sent ack" << std::endl;
}