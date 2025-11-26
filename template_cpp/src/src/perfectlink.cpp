#include "perfectlink.hpp"
using namespace std;

// Put the message to outbox_. When
void PerfectLink::addSendlist(Parser::Host dest, Msg msg)
{
    lock_guard<mutex> lock(mu_);
    sendlist_.push_back(make_tuple(dest, msg));
}

void PerfectLink::send(int sockfd, Parser::Host dest, Msg msg)
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
    std::cout << "sent message: " << serialized_msg << std::endl;
}

// resend unacked messages
void PerfectLink::resend(int sockfd)
{
    std::cout << "resend message(s)" << std::endl;
    lock_guard<mutex> lock(mu_);
    for (auto &item : sendlist_)
    {
        send(sockfd, get<0>(item), get<1>(item));
    }
}

void PerfectLink::onPacketReceived(int sockfd, Parser::Host myHost, Parser::Host srcHost, Msg msg)
{
    lock_guard<mutex> lock(mu_);

    if (msg.type == MessageType::ACK)
    {
        for (auto it = sendlist_.begin(); it != sendlist_.end();)
        {
            // delete item from sendlist
            if (get<0>(*it).srcId == srcHost.srcId && get<1>(*it).m == msg.m)
            {
                it = sendlist_.erase(it);
                std::cout << "ACK Received for seq " << msg.m << ". Removed from list." << std::endl;
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
        Msg ackMsg(MessageType::ACK, myHost.srcId, myHost.srcId, msg.m);

        sendAck(sockfd, srcHost, ackMsg);
    }
}

void PerfectLink::sendAck(int sockfd, Parser::Host srcHost, Msg ackMsg)
{
    send(sockfd, srcHost, ackMsg);
    std::cout << "Sent ack" << std::endl;
}