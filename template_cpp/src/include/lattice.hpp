#pragma once
#include <set>
#include <functional>
#include "peer.hpp"

typedef std::set<int> proposalSet;

enum class LatticeMessageType
{
    PROPOSAL,
    ACK,
    NACK,
};
inline string enum_to_string(LatticeMessageType type)
{
    return to_string(static_cast<int>(type));
}

// data used in Lattice Agreement
struct LatticePayload
{
    LatticeMessageType type;
    proposalSet proposed_value;
    int active_proposal_number;

    // PROPOSAL:{1,2,3,}:1
    string serialize() const
    {
        string res = "";
        res += enum_to_string(type) + ":{";
        for (auto &val : proposed_value)
        {
            res += val + ",";
        }
        res += "}:" + to_string(active_proposal_number);
    };

    void deserialize(string payload_m)
    {
        // type
        auto delim1 = payload_m.find(':');
        this->type = static_cast<LatticeMessageType>(stoi(payload_m.substr(0, delim1)));

        string remaining_str = payload_m.substr(delim1 + 1);

        // proposed_value
        auto delim2 = remaining_str.find(':');
        string proposed_value_string = remaining_str.substr(0, delim2);
        if (proposed_value_string != "")
        {
            // todo: add each element to set
        }

        // active_proposal_number
        string payload_str = remaining_str.substr(delim2 + 1);
    };
};

class LatticeProposer
{
public:
    LatticeProposer(int f, function<void(LatticePayload)> broadcastCallback)
        : f_(f), broadcastPayloadCallback_(broadcastCallback)
    {
        Init();
    }
    void Init();
    void Propose(proposalSet proposal);
    void Receive(const LatticePayload &p);

private:
    bool active_;
    int ack_count_;
    int nack_count_;
    // active_proposal_number_ denotes the number used to distinguish between proposals WITHIN this shot (a single line of proposal in config)
    int active_proposal_number_;
    // todo: think of content of set
    proposalSet proposed_value_;

    // n = 2f + 1
    int f_;

    // broadcast (using broadcastCallback)
    function<void(LatticePayload)> broadcastPayloadCallback_;
};

class LatticeAcceptor
{
public:
    LatticeAcceptor(function<void(LatticePayload)> sendCallback)
        : sendPayloadCallback_(sendCallback)
    {
        Init();
    }
    void Init();
    void Receive(const LatticePayload &p);

private:
    proposalSet accepted_value_;
    function<void(LatticePayload)> sendPayloadCallback_;
};